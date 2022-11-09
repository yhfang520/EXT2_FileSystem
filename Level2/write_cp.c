/************* write_cp.c file **************/

#ifndef __WRITECP_C__
#define __WRITECP_C__

#include "read_cat.c"
#include "link_unlink.c"

int write_file(int fd, char *buf)
{
    return 1; // Eventually: return the results of my_write
}

int my_write(int fd, char buf[], int nbytes)
{
    return 1; // Eventually: return the number of bytes written
}

int my_cp(char *pathname)
{
    return 1;
}

#endif