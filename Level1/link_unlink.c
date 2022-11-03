/************* link_unlink.c file **************/

#ifndef __LINKUNLINK_C__
#define __LINKUNLINK_C__

#include "mkdir_creat.c"
#include "rmdir.c"

int my_link(MINODE *filePip, char *fileName, int fileIno, MINODE *linkPip, char *linkName)
{
    return 1;
}

int link_file(char *pathname)
{
    return 1;
}

int my_unlink(char *pathname)
{
    return 1;
}

int my_rm(MINODE *mip, char *pathname)
{
    return 1;
}

#endif