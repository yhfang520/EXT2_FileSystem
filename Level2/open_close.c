/************* open_close_lseek.c file **************/

#ifndef __OPENCLOSELSEEK_C__
#define __OPENCLOSELSEEK_C__

#include "../Level1/mkdir_creat.c"

int truncate(MINODE *mip)
{
  return 1;
}

int open_file(char *pathname, int mode)
{
  return 1; // Eventually: return file descriptor of opened file
}

int my_close(int fd)
{
  return 1;
}

int close_file(int fd)
{
  return 0;
}

int my_lseek(int fd, int position)
{
  return 1; // Eventually: return original position in file
}

int pfd()
{
    return 1;
}

int dup(int fd)
{
    return 1;
}

int dup2(int fd, int gd)
{
    return 1;
}

#endif