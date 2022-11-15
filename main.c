/****************************************************************************
*                   KCW: mount root file system                             *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <ext2fs/ext2_fs.h>

#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"

extern MINODE *iget();

MINODE  minode[NMINODE];
MINODE  *root;
PROC    proc[NPROC], *running;
MTABLE  mtable[NMTABLE];
OFT     oft[NOFT];

char gpath[128]; // global for tokenized components
char *name[64];  // assume at most 64 components in pathname
int   n;         // number of component strings

int  fd, dev, rootdev;
int  nblocks, ninodes, bmap, imap, iblk, mode;
char line[128], cmd[32], pathname[128], parameter[128];

#include "alloc_dealloc.c"

#include "Level1/cd_ls_pwd.c"
#include "Level1/mkdir_creat.c"
#include "Level1/rmdir.c"
#include "Level1/link_unlink.c"
#include "Level1/symlink.c"
#include "Level2/open_close.c"

int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;

  printf("init()\n");

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->pid = i+1;           // pid = 1, 2
    p->uid = p->gid = 0;    // uid = 0: SUPER user
    p->cwd = 0;             // CWD of process
  }
  for (i=0; i<NMTABLE; i++){
    mtable[i].dev=0;
  }
}

// load root INODE and set root pointer to it
int mount_root()
{  
  printf("mount_root()\n");
  root = iget(dev, 2);
}

char *disk = "mydisk";     // change this to YOUR virtual

int main(int argc, char *argv[ ])
{
  int ino;
  char buf[BLKSIZE];

  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }

  dev = rootdev = fd;    // global dev same as this fd   

  /********** read super block  ****************/
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  /* verify it's an ext2 file system ***********/
  if (sp->s_magic != 0xEF53){
      printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
      exit(1);
  }     
  printf("EXT2 FS OK\n");
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  get_block(dev, 2, buf); 
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  iblk = gp->bg_inode_table;
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, iblk);

  init();  
  mount_root();
  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->cwd = iget(dev, 2);
  printf("root refCount = %d\n", root->refCount);

  // WRTIE code here to create P1 as a USER process
  
  while(1){
    printf("input command : [ls|cd|pwd|mkdir|creat|rmdir|link|symlink|unlink|readlink|open|close|lseek|quit] ");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    if (line[0]==0)
       continue;
    pathname[0] = 0;
    parameter[0] = 0;

    sscanf(line, "%s %s %s", cmd, pathname, parameter);
    printf("cmd=%s pathname=%s parameter=%s\n", cmd, pathname, parameter);

    sscanf(line, "%s %s %d", cmd, pathname, &mode);

    if (strcmp(cmd, "ls")==0)
       ls();
    else if (strcmp(cmd, "cd")==0)
       cd(pathname);
    else if (strcmp(cmd, "pwd")==0)
       pwd(running->cwd);
    else if(strcmp(cmd, "mkdir")==0)
      make_dir(pathname);
    else if(strcmp(cmd, "creat")==0)
      creat_file(pathname); 
    else if (strcmp(cmd, "rmdir")==0)
      remove_dir(pathname);
    else if (strcmp(cmd, "link")==0)
      my_link(pathname, parameter);
    else if (strcmp(cmd, "symlink")==0)
      //printf("%s %s\n", pathname, parameter); // for testing
      symlink_file(pathname, parameter);
    else if (strcmp(cmd, "unlink")==0)
      my_unlink(pathname);
    else if (strcmp(cmd, "readlink")==0)
      my_readlink(pathname);
    else if (strcmp(cmd, "open")==0)
      open_file(pathname, mode);
    else if (strcmp(cmd, "close")==0)
      close_file(pathname);
    else if (strcmp(cmd, "quit")==0)
       quit();
  }
}

int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  printf("see you later, alligator\n");
  exit(0);
}