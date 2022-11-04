/************* symlink.c file **************/

#ifndef __SYMLINK_C__
#define __SYMLINK_C__


#include "mkdir_creat.c"


extern MINODE *iget();

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;

extern char gpath[128]; // global for tokenized components
extern char *name[64];  // assume at most 64 components in pathname
extern int   n;         // number of component strings

extern int  fd, dev;
extern int  nblocks, ninodes, bmap, imap, iblk;
extern char line[128], cmd[32], pathname[128];


int symlink_file(char *pathname, char *parameter)
{
    // Check if the file exists
    int oino = getino(pathname);
    if (oino == 0)
    {
        printf("File does not exist\n");
        return -1;
    }
    // Check if the parameter exists
    int nino = getino(parameter);
    if (nino != 0)
    {
        printf("New file exists\n");
        return -1;
    }
    // Create the new file
    creat_file(parameter);

    // Get the new file's inode
    nino = getino(parameter);
    MINODE *mip = iget(dev, nino);

    // Set the new file's type to symbolic link
    mip->INODE.i_mode = 0120000;

    // assume length of pathname is <= 60 chars
    if (strlen(pathname) > 60)
    {
        printf("Pathname too long\n");
        return -1;
    }

    // store pathname in parameter INODE.i_block[0]
    strcpy((char *)mip->INODE.i_block, pathname);

    // Set the new file's size length of parameter
    mip->INODE.i_size = strlen(pathname);

    // Mark the new file as dirty
    mip->dirty = 1;

    iput(mip);// write INODE back to disk
    return 1;
}

int my_readlink(char *pathname)
{
    return 1;
}

#endif