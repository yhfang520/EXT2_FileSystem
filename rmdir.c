/************* rmdir.c file **************/

#ifndef __RMDIR_C__
#define __RMDIR_C__

#include "open_close.c"

//11.8.4
int rm_child(MINODE *pip, char *name)
{
    DIR *dp;
    char buf[BLKSIZE], name[256];
    //get in-memory INODE of pathname 
    int ino = getino(pathname);
    if (ino == -1){
        printf("Access denied\n");
        return 0; 
    }
    MINODE *mip = iget(dev, ino); 
    findmyname(mip, ino, name); 
    
    //verify INODE is a DIR (by INODE.i_mode field); minodes is not BUSY (refCount = 1)
    if ((mip->INODE.i_mode & 0xF000) == 0x4000 && mip->refCount = 1){  //is (S_ISDIR())
      if (mip->inode.i_links_count = 2){   //verify DIR is empty (traverse data blocks for number of entries = 2);
        get_block(dev, mip->inode.i_block[0], buf); 

        //get parent's ino and inode
        u32 *inum = malloc(8);  
        int pino = findino(mip, inum);  //get pino from .. entry in INODE.i_block[0]
      }
    }

    return 1;
}

int remove_dir(char *pathname)
{
    return 1;
}

#endif