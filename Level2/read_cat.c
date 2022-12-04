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
  if((running->fd[fd]->mode == 0) && (running->fd[fd]->mode) == 2)
    return (my_read(fd, buf, nbytes));
  else 
    printf("File is not open for read\n");
  
  printf("--------------------------------------------\n");
  printf("\n%s \n", buf);
  printf("--------------------------------------------\n");
  return 1;
}

int my_read(int fd, char buf[], int nbytes)
{
  OFT *oftp = running->fd[fd];
  MINODE *mip = oftp->mptr;
  int count = 0, lbk, startByte, blk, avil, remain, dblk;
  int ibuf[256], buf13[256], dbuf[256], ebuf[BLKSIZE];
  avil = mip->INODE.i_size - oftp->offset;
  char *cq = buf, *cp;
  char readbuf[BLKSIZE];
  
  while(nbytes && avil)
  {
    lbk = oftp->offset / BLKSIZE;
    startByte = oftp->offset % BLKSIZE;

    if(lbk < 12) // direct block
    {
      blk = mip->INODE.i_block[lbk];
    }
    else if(lbk >= 12 && lbk < 256 + 12) // indreact block
    {
      //printf("\nmyread: indirect blocks\n");
      get_block(mip->dev, mip->INODE.i_block[12], (char *)ibuf); // get block into memory
      blk = ibuf[lbk - 12];
    }
    else{ // double indirect block
      //printf("\nmyread: double indirect blocks\n");
      get_block(mip->dev, mip->INODE.i_block[13], (char *)ibuf);  //get block into memory
      //kc mail man algorithm 11.3.1
      lbk -= (12 + 256);
      blk = ibuf[lbk/256];
      get_block(mip->dev, blk, (char *)buf13);
      blk = buf13[lbk % 256];
    }
    get_block(mip->dev, blk, readbuf);
    char *cp = readbuf + startByte;
    remain = BLKSIZE - startByte;

    while (remain > 0){
      if (nbytes <= avil && nbytes <= remain){
        strncpy(cq, cp, nbytes);
        oftp->offset = oftp->offset + nbytes;
        count = count + nbytes;
        avil = avil - nbytes;
        nbytes = 0;
        remain = remain - nbytes; 
      } else if (avil <= remain && avil <= nbytes){
        strncpy(cq, cp, avil);
        oftp->offset = oftp->offset + avil;
        count = count + avil;
        avil = 0;
        nbytes = nbytes - avil;
        remain = remain - avil; 
      } else{
        strncpy(cq, cp, remain);
        oftp->offset = oftp->offset + remain;
        count = count + remain;
        avil = avil - remain;
        nbytes = nbytes - remain;
        remain = 0; 
      }
      if (nbytes <= 0 || avil <= 0){
        break;
    }
    }
  }
  return count; // Eventually: Return the actual number of bytes read
}

int cat_file(char *pathname)
{
  char mybuf[1024];
  int dummy = 0;
  int n = 0, fd, i;
  fd = open_file(pathname, 0);

  //check fd is valid
  if(fd < 0){
    printf("cat: open file failed\n");
    return -1;
  }

  // for(n = 0; my_read(fd, mybuf, 1024); n++){
  //   mybuf[n] = 0;
  //   printf("%s", mybuf);
  // }
  
  while(n = my_read(fd, mybuf, 1024)){
    mybuf[n] = 0;
    for (i = 0; i < n; i++){
      if (mybuf[i] == '\\' && mybuf[i++] == 'n'){
        printf("\n");
      }
    }
    printf("%s", mybuf);
  }
  
  close_file(fd);
  return 1;
}

#endif