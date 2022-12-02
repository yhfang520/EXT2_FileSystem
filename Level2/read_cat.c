/************* read_cat.c file **************/

#ifndef __READCAT_C__
#define __READCAT_C__

#include "open_close.c"

int read_file()
{
  int fd = 0, nbytes = 0;
  char buf[BLKSIZE];
  pfd();

  printf("enter fd = ");
  scanf("%d", &fd);
  printf("enter nbytes = ");
  scanf("%d", &nbytes);

  // check fd is valid
  if (fd < 0 || fd > NFD)
  {
    printf("fd is not valid \n");
    return -1;
  }

  // check if fd is open for reading
  if(running->fd[fd]->mode == 0 || running->fd[fd]->mode == 2)
    return (my_read(fd, buf, nbytes));
  else 
    printf("File is not open for read\n");
 
  return 1; // Eventually: Return the results of my_read
}

int my_read(int fd, char buf[], int nbytes)
{
  OFT *oftp = running->fd[fd];
  MINODE *mip = oftp->mptr;
  int count = 0, lbk, startByte, blk, avil, remain;
  int ibuf[256];
  avil = mip->INODE.i_size - oftp->offset;
  char *cq = buf;
  char readbuf[BLKSIZE];

  while(nbytes && avil)
  {
    // nbytes test
    //printf("%d \n", nbytes);
    lbk = oftp->offset / BLKSIZE;
    startByte = oftp->offset % BLKSIZE;

    if(lbk < 12) // direct block
    {
      blk = mip->INODE.i_block[lbk];
    }
    else if(lbk >= 12 && lbk < 256 + 12) // indreact block
    {
      get_block(mip->dev, mip->INODE.i_block[12], ibuf); // get block into memory
      blk = ibuf[lbk - 12]; 
    }
    else{ // double indirect block
      get_block(mip->dev, mip->INODE.i_block[13], ibuf);
      blk = ibuf[(lbk - 12 - 256) / 256];
      get_block(mip->dev, blk, ibuf);
      blk = ibuf[(lbk - 12 - 256) % 256];
    }
    get_block(mip->dev, blk, readbuf);
    char *cp = readbuf + startByte;
    remain = BLKSIZE - startByte;
    while(remain > 0){
      *cq++ = *cp++;
      oftp->offset++;
      count++;
      avil--; nbytes--; remain--;
      if(nbytes <= count || avil <= 0){
        break;
      }
    }
    
  }

  printf("--------------------------------------------\n");
  printf("\n%s \n", buf);
  printf("--------------------------------------------\n");
  printf("myread: read %d char from file descriptor %d\n", count, fd);
  return count; // Eventually: Return the actual number of bytes read
}

int cat_file(char *pathname)
{
  char mybuf[1024];
  int dummy = 0;
  int n = 0, fd;
  fd = open_file(pathname, 0);

  //check fd is valid
  if(fd < 0){
    printf("cat: open file failed\n");
    return -1;
  }

  while(n = my_read(fd, mybuf, 1024)){
    mybuf[n] = 0;
    printf("test\n");
    printf("%s", mybuf);
  }
  close_file(fd);
  return 1;
}

#endif