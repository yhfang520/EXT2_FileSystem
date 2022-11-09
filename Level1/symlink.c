/************* symlink.c file **************/

#ifndef __SYMLINK_C__
#define __SYMLINK_C__


#include "mkdir_creat.c"


int symlink_file(char *pathname, char *parameter)
{
    // check if pathname exists
    int ino = getino(pathname);
    if (ino == -1){
        printf("symlink: %s does not exist\n", pathname);
        return -1;
    }

    // check if parameter exists
    int nino = getino(parameter);
    if (nino != -1){
        printf("symlink: %s already exists\n", parameter);
        return -1;
    }
    // creat a file named parameter
    creat_file(parameter);

    // set the parameter's INODE.i_mode to LNK
    int pino = getino(parameter);
    MINODE *mip = iget(dev, pino);
    mip->INODE.i_mode = 0xA1FF;
    // set INODE.i_links_count to 1
    mip->INODE.i_links_count = 1;
    // set INODE.i_block to 0
    mip->INODE.i_blocks = 0;

    // check the length of the pathname <= 60
    if (strlen(pathname) >= 60){
        printf("symlink: pathname is too long\n");
        return -1;
    }

    // store the pathname name in the INODE.i_block[0]
    strncpy((char *)mip->INODE.i_block, pathname, strlen(pathname));
    // set the INODE.i_size to the length of the pathname
    mip->INODE.i_size = strlen(pathname);
    mip->dirty = 1;
    iput(mip);

    return 1;
}

int my_readlink(char *pathname)
{
    // get file inode in memory 
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino);
    
    //check if it is a symbolic link
    if (!S_ISLNK(mip->INODE.i_mode)){
        printf("readlink: %s is not a symbolic link\n", pathname);
        return -1;
    }

    // copy the pathname from the INODE.i_block[0] to buf[ ] using strncpy()
    char buf[BLKSIZE];
    strncpy(buf, (char *)mip->INODE.i_block, BLKSIZE);
    // print the pathname
    printf("%s\n", buf);


    // return the length of the pathname
    return strlen(buf);

}

#endif