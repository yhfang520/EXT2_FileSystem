


extern MINODE *iget();

MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;

char gpath[128]; // global for tokenized components
char *name[64];  // assume at most 64 components in pathname
int   n;         // number of component strings

int  fd, dev;
int  nblocks, ninodes, bmap, imap, iblk;
char line[128], cmd[32], pathname[128];



/************* cd_ls_pwd.c file **************/
int cd(char *pathname)
{
  int ino;
  MINODE *mip;
  printf("cd %s\n", pathname);
  if (pathname[0]==0) // cd to root 
    mip = root;
  else{
    ino = getino(pathname); // get ino of pathname
    if (ino==0){ // pathname does not exist
      printf("no such file %s\n", pathname);
      return -1;
    }
    mip = iget(dev, ino); // get its minode
  }
  iput(running->cwd); // dispose of old minode
  running->cwd = mip; // new minode becomes CWD
}


int ls_file(MINODE *mip, char *name)
{
  // printf("ls_file: to be done: READ textbook!!!!\n");
  // READ Chapter 11.7.3 HOW TO ls
  printf("\n");

  char *t1 = "xwrxwrxwr-------";
  char *t2 = "----------------";

  int i;
  char *ftime;
  INODE *inode = &mip->INODE;

  printf("%-3d ", inode->i_links_count);
  if (S_ISDIR(inode->i_mode))
    printf("d");
  else if (S_ISREG(inode->i_mode))
    printf("-");
  else if (S_ISLNK(inode->i_mode))
    printf("l");
  
  for (i=8; i >= 0; i--){
    if (inode->i_mode & (1 << i))
      printf("%c", t1[i]);
    else
      printf("%c", t2[i]);
  }

  printf("%4d ", inode->i_uid);
  printf("%4d ", inode->i_gid);
  printf("%8d ", inode->i_size);
  printf("%4d ", inode->i_links_count);
  time_t t = inode->i_ctime; // time in seconds
  ftime = ctime(&t); // convert to calendar time
  ftime[strlen(ftime)-1] = 0; // kill \n at end
  printf("%s ", ftime);
  printf("%s", basename(name)); // print file basename
  if(S_ISLNK(inode->i_mode)){
    char buf[BLKSIZE];
    readlink(mip, buf); 
    printf(" -> %s", buf);
  }
  printf(" [%d %2d] ", mip->dev, mip->ino); // print [dev, ino]
}

int ls_dir(MINODE *mip)
{
  //printf("ls_dir: list CWD's file names; YOU FINISH IT as ls -l\n");
  //printf("\n");
  int i;
  char buf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;
  MINODE *dip;

  get_block(mip->dev, mip->INODE.i_block[0], buf); // get data block into buf
  dp = (DIR *)buf;
  cp = buf;
  printf("i_block[0] = %d\n", i, mip->INODE.i_block[0]);
  //printf("TESTESTTES\n");
  while (cp < buf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len); // copy name[ ] into temp[ ]
     temp[dp->name_len] = 0; 

     dip = iget(dev, dp->inode); // get the minode of the file
     ls_file(dip, temp); // print the file's info
     cp += dp->rec_len; // move to the next entry
     dp = (DIR *)cp; // cast the dp to the next entry
  }

  printf("\n");
}

int ls()
{
  int ino;
  MINODE *mip;
  printf("ls %s\n", pathname);
  if (pathname[0]==0) // ls CWD
    ls_dir(running->cwd); 
  else{
    ino = getino(pathname); // get ino of pathname
    if (ino==0){ // pathname does not exist
      printf("no such file %s\n", pathname);
      return -1;
    }
    mip = iget(dev, ino); // get its minode
    if (S_ISDIR(mip->INODE.i_mode)) // if it's a directory
      ls_dir(mip); 
    else
      ls_file(mip, basename(pathname));
    iput(mip); // dispose of memory
  }
  printf("\n");
}

char *pwd(MINODE *wd)
{
  printf("pwd: READ HOW TO pwd in textbook!!!!\n");
  if (wd == root){
    printf("/\n");
    return;
  }
  rpwd(wd);
  printf("\n");
  
}

void rpwd(MINODE *wd)
{
  if (wd == root){
    return;
  }
  int ino, pino;
  char buf[BLKSIZE], temp[256];
  get_block(dev, wd->INODE.i_block[0], buf); // get the data block of the wd
  pino = findino(wd, &ino); // get the parent ino
  MINODE *pip = iget(dev, pino); // find the parent in memory
  findmyname(pip, ino, temp); // find the name of the wd in the parent
  rpwd(pip); // recursively call rpwd on the parent
  printf("/%s", temp);
}


