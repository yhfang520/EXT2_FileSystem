/*********** util.c file ****************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
extern MTABLE  mtable[NMTABLE];
extern OFT     oft[NOFT];

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd, dev, rootdev;
extern int nblocks, ninodes, bmap, imap, iblk;
extern char line[128], cmd[32], pathname[128], parameter[128];

int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}   

int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}   

int tokenize(char *pathname)
{
  int i;
  char *s;
  printf("tokenize %s\n", pathname);

  strcpy(gpath, pathname);   // tokens are in global gpath[ ]
  n = 0;

  s = strtok(gpath, "/");
  while(s){
    name[n] = s;
    n++;
    s = strtok(0, "/");
  }
  name[n] = 0;
  
  for (i= 0; i<n; i++)
    printf("%s  ", name[i]);
  printf("\n");
}

// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino)
{
  int i;
  MINODE *mip;
  char buf[BLKSIZE];
  int blk, offset;
  INODE *ip;

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->dev==dev && mip->ino==ino){
       mip->refCount++;
       //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount == 0){
       //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev;
       mip->ino = ino;

       // get INODE of ino into buf[ ]    
       blk    = (ino-1)/8 + iblk;
       offset = (ino-1) % 8;

       //printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

       get_block(dev, blk, buf);    // buf[ ] contains this INODE
       ip = (INODE *)buf + offset;  // this INODE in buf[ ] 
       // copy INODE to mp->INODE
       mip->INODE = *ip;
       return mip;
      } 
   }
   printf("PANIC: no more free minodes\n");
   return 0; 
}

void iput(MINODE *mip)  // iput(): release a minode
{
 int i, block, offset;
 char buf[BLKSIZE];
 INODE *ip;

 if (mip==0) 
     return;

 mip->refCount--; //dec refCount by 1
 
 if (mip->refCount > 0) return;  //still has user 
 if (!mip->dirty)       return;  //INODE had not changed; no need to write back 
 
 /* write INODE back to disk */
 /**************** NOTE ******************************
  For mountroot, we never MODIFY any loaded INODE
                 so no need to write it back
  FOR LATER WROK: MUST write INODE back to disk if refCount==0 && DIRTY

  Write YOUR code here to write INODE back to disk
 *****************************************************/

 block = (mip->ino - 1) / 8 + iblk; 
 offset = (mip->ino - 1) % 8;

 // get block containning this inode 
 get_block(mip->dev, block, buf);
 ip = (INODE *)buf + offset;  //ip points at INODE 
 *ip = mip->INODE;   //copy INODE to inode in block
 put_block(mip->dev, block, buf);   //write back to disk 
 mip->refCount = 0;   //mip->redCount = 0;  
} 

int search(MINODE *mip, char *name)
{
   int i; 
   char *cp, c, sbuf[BLKSIZE], temp[256];
   DIR *dp;
   // INODE *ip;

   /*** search for name in mip's data blocks: ASSUME i_block[0] ONLY ***/
   printf("search for %s in MINODE = [%d, %d]\n", name, mip->dev, mip->ino);
   // ip=&(mip->inode);

   for (i=0; i < 12; i++){
      if (mip->INODE.i_block[i]==0)
         return 0; 
      
      get_block(mip->dev, mip->INODE.i_block[0], sbuf);
      dp = (DIR *)sbuf;
      cp = sbuf;
      printf("search: i=%d i_block[%d]=%d\n", i, i, mip->INODE.i_block[0]);
      printf("  i_number  rec_len  name_len    name\n");

      while (cp < sbuf + BLKSIZE){
         strncpy(temp, dp->name, dp->name_len); // dp->name is NOT a string
         temp[dp->name_len] = 0;                // temp is a STRING
         printf("%4d        %4d      %4d       %s\n", 
            dp->inode, dp->rec_len, dp->name_len, temp); // print temp !!!

         if (strcmp(temp, name)==0){            // compare name with temp !!!
            printf("found %s : ino = %d\n", temp, dp->inode);
            return dp->inode;
         }

         cp += dp->rec_len;
         dp = (DIR *)cp;
      }
   }
   return 0;
}

MTABLE *getmtable(int dev)
{
   MTABLE *nm;
   for (int i=0; i < NMTABLE; i++){
      nm = &mtable[i];
      if (nm->dev == dev){
         return nm; 
      }
   }
   return 0; 
}

int getino(char *pathname) // return ino of pathname   
{
  int i, ino, blk, offset;
  char buf[BLKSIZE];
  INODE *ip;
  MINODE *mip;
  MTABLE *mp; 


  printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/")==0)
      return 2;   //return root ino = 2
  
  // starting mip = root OR CWD
  if (pathname[0]=='/'){
      dev = root->dev;  //if absolute pathname: start from root
      ino = root->ino; 
   } else{
     //mip = running->cwd;   //if relative pathname: start from CWD
      dev = running->cwd->dev; 
      ino = running->cwd->ino; 
   }
  mip = iget(dev, ino);         // because we iput(mip) later
  //mip->refCount++;  
  //mip->refCount+=1;
  tokenize(pathname);

  for (i=0; i < n; i++){   //search for each component string 
      printf("===========================================\n");
      printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);

      if (!S_ISDIR(mip->INODE.i_mode)){   //check DIR type
         printf("%s is not a directory\n", name[i]);
         iput(mip);
         return 0; 
      }

      //To-do upward traversal

      ino = search(mip, name[i]);

      if (!ino){
         iput(mip);
         printf("name %s does not exist\n", name[i]);
         return -1;
      }

      iput(mip);  //realease current minode 
      mip = iget(dev, ino);   //switch to new minode 

      // //check downward cross mouting point 
      // if (mip->mounted){   //downward direction 
      //    MTABLE *mtptr = mip->mptr;
      //    dev = mtptr->dev;
      //    iput(mip);  //release current mip
      //    mip = iget(dev, 2);  //switch to mounting root 
      //    ino = 2; 
      // }

      if (ino==2 && dev != rootdev){   //if not the initial device 
         if (i > 0){
            printf("crossing down to mount point\n"); 
            mp = getmtable(dev);
            mip = mp->mntDirPtr;
            dev = mip->dev;  
         }
         i++; 
      }
      else if (mip->mounted){
         mp = mip->mptr;
         if (dev != mp->dev){
            dev = mp->dev; //update global device 
            mip = iget(dev, 2);  //get root of new mount 
            ino = 2; 
         }
      }
   }
   iput(mip);
   return ino;
}

// These 2 functions are needed for pwd()
int findmyname(MINODE *parent, u32 myino, char myname[ ]) 
{
  // WRITE YOUR code here
  // search parent's data block for myino; SAME as search() but by myino
  // copy its name STRING to myname[ ]

   char *cp, c, sbuf[BLKSIZE], temp[256];
   DIR *dp;
   INODE *ip;


   get_block(dev, parent->INODE.i_block[0], sbuf); // get the block
   dp = (DIR *)sbuf; // cast the block to a DIR
   cp = sbuf; // set cp to the beginning of the block
   printf("  i_number  rec_len  name_len  name\n");

   while (cp < sbuf + BLKSIZE){ 
      strncpy(temp, dp->name, dp->name_len); // dp->name is NOT a string
      temp[dp->name_len] = 0;                // temp is a STRING
      printf("  %4d     %4d     %4d        %s\n", 
          dp->inode, dp->rec_len, dp->name_len, temp); // print temp !!!
 
      if (dp->inode == myino){            // compare name with temp !!!
         //printf("found %s ino = %d\n", temp, dp->inode);
         strcpy(myname, temp);
         return 1;
      }

      cp += dp->rec_len;
      dp = (DIR *)cp;
   }
   
   
}

int findino(MINODE *mip, u32 *myino) // myino = i# of . return i# of ..
{
  // mip points at a DIR minode
  // WRITE your code here: myino = ino of .  return ino of ..
  // all in i_block[0] of this DIR INODE.

   char *cp, c, sbuf[BLKSIZE], temp[256]; 
   DIR *dp;
   INODE *ip;

   get_block(dev, mip->INODE.i_block[0], sbuf); // get the block
   dp = (DIR *)sbuf; // cast the block to a DIR
   cp = sbuf; // set cp to the beginning of the block
   //printf("  i_number  rec_len  name_len  name\n");

   while (cp < sbuf + BLKSIZE){ 
      strncpy(temp, dp->name, dp->name_len); // dp->name is NOT a string
      temp[dp->name_len] = 0;                // temp is a STRING
      //printf("%4d  %4d  %4d    %s\n", 
        // dp->inode, dp->rec_len, dp->name_len, temp); // print temp !!!

      if (strcmp(temp, ".") == 0){  // if the name is . set myino to the inode
         //printf("found %s ino = %d\n", temp, dp->inode);
         *myino = dp->inode; // set myino to the inode 
      }
      else if (strcmp(temp, "..") == 0) // if the name is .. return to the inode
      {
         //printf("found %s ino = %d\n", temp, dp->inode);
         return dp->inode; // return the inode
      }

      cp += dp->rec_len; // increment cp by the rec_len
      dp = (DIR *)cp; // cast cp to a DIR
   }

}
