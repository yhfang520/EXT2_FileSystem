/************* open_close_lseek.c file **************/

#ifndef __OPENCLOSELSEEK_C__
#define __OPENCLOSELSEEK_C__

#include "../Level1/mkdir_creat.c"

int truncate(MINODE *mip)
{
  INODE *ip = &mip->INODE; 
  int i, count=0, double_count = 0;;
  char buf[BLKSIZE];
  int ibuf[256], dbuf[256], ddbuf[256];

  //Refernce by KC : )
  //iterate through blocks 
  for (i=0; i < 12; i++){ //print all 15 i_block[] numbers; first 12 are DIRECT blocks 
    printf("i_block[%d] = %d\n", i, ip->i_block[i]);
    if (ip->i_block[i] == 0)
      break; 
    bdalloc(dev, ip->i_block[i]);
    ip->i_block[i] = 0;
    // bzero(ip->i_block, BLKSIZE);  be zero if it's not empty 
  }
  
  if (ip->i_block[12] != 0){  //indirect blocks
    printf("------- INDIRECT BLOCKS -------\n");
    get_block(dev, ip->i_block[12], (char *)ibuf);  //ibuf[0] ibuf[1] 
    while (count < 256){
      if (ibuf[count] == 0)
        break; 
      bdalloc(dev, ibuf[count]);
      ibuf[count] = 0; 
      count++;   
    }
    bdalloc(dev, ip->i_block[12]); 
    ip->i_block[12] = 0; 
  } 
  
  if (ip->i_block[13] != 0){  //double indirect blocks
    printf("------- DOUBLE INDIRECT BLOCKS -------\n");
    get_block(dev, ip->i_block[13], (char *)dbuf);  //dbuf=dbuf[0] dbuf[1] dbuf[2] each one is an number 
    while (count < 256){
      if (dbuf[count] == 0)
        break; 
      get_block(dev, dbuf[count], (char *)ddbuf); //get number into ddbuf[]
      while (double_count < 256){
        if (ddbuf[double_count] == 0)
          break; 
        bdalloc(dev, ddbuf[double_count]);
        double_count++;     
      }
    }
    bdalloc(dev, ip->i_block[13]);
    ip->i_block[13] = 0;
  }
  
  mip->INODE.i_blocks = 0;
  //set INODE's size to 0 and mark Minode[ ] dirty
  mip->INODE.i_size = 0;
  mip->dirty = 1; 
  iput(mip);

  return 1;
}

//11.9.1
int open_file(char *pathname, int mode)
{
  int ino, i, pino, r, descriptor=-1;
  MINODE *mip, *pmip;
  OFT *oftp; 

  if (pathname[0] == '/')
    dev = root->dev;
  else 
    dev = running->cwd->dev;
  
  //get pathname's inumber, minode pointer 
  ino = getino(pathname); // ino = getino(pathname, &dev);

  if (ino == 0){  //if file does not exist  
  printf("file not exist\n"); 
    creat_file(pathname); 
    ino = getino(pathname); // ino = getino(pathname, &dev); 
  }

  mip = iget(dev, ino); //get the ino and mip of pathname  

  if (!S_ISREG(mip->INODE.i_mode)){
    printf("WARNING: %s is not a regular file\n", pathname);
    return -1; 
  }

  //check whether the file is ALREADY opened with INCOMPATABLE mode 
  for (i=0; i < NFD; i++){
    if (running->fd[i] != NULL && running->fd[i]->mptr == mip){
      if (mode != 0){
        printf("%s is already opened with incompatable mode\n", pathname);
        return -1;
      }
    } 
  }

  //allocate a FREE OpenFileTable(OFT) and fill in values 
  oftp = (OFT *)malloc(sizeof(OFT)); //build the open fd 
  oftp->mode = mode;  
  oftp->refCount = 1;
  oftp->mptr = mip;  

  printf("%s opend in ", pathname);
  switch (mode){  //offset
    case 0: //Read: offset=0
      printf("read");
      oftp->offset = 0;
      break; 
    case 1: //Write: truncate file to 0 sizes 
      printf("write");
      truncate(mip);
      oftp->offset = 0;
      break; 
    case 2: //Read/Write: do NOT truncate file, offset = 0
      printf("read/write");
      oftp->offset = 0;
      break; 
    case 3: //Append: offset to size of file 
      oftp->offset = mip->INODE.i_size; //APPEND mode  
    default:
      printf("invalid mode\n");
      return -1; 
  }

  printf(" mode = %d: fd = %d\n", mode, running->fd[i]);
  
  for (i=0; i < NFD; i++){  //find empty fd in running PROC's fd array 
    if (running->fd[i] == NULL){
      running->fd[i] = oftp;  //assign the OFT to the fd[i] this was assigned to open
      descriptor = i;
      break; 
    }
  }
  
  //update INODE's time filed 
  if (mode > 0){ //if not R touch atime and mtime 
    mip->INODE.i_mtime = time(0L);
  }
  mip->INODE.i_atime = time(0L);  //update inode access time 
  mip->dirty = 1;
  //iput(mip);
  printf("%s opened in mode %d\n", pathname, mode);
  return descriptor; // Eventually: return file descriptor of opened file
}

int close_file(int fd)
{
  //check fd is a valid opened file descriptor
  if (fd < 0 || fd >= NFD){
    printf("fd not in range\n");
    return -1; 
  }
  //check pointing at OFT entry 
  if (running->fd[fd] == NULL){
    printf("not OFT entry\n");
    return -1; 
  }
  OFT *oftp = running->fd[fd];
  running->fd[fd] = 0;  //clear PROC's fd[fd] to 0
  oftp->refCount--; //dec OFT's refCount by 1
  if (oftp->refCount > 0)
    return 0;
  MINODE *mip = oftp->mptr;
  mip->dirty = 1; 
  iput(mip);  //release minode 
  free(oftp);
  printf("close: refCount = %d\n", oftp->refCount++); 
  printf("fd = %d is closed\n", running->fd[fd]); 
  return 0;
}

int my_lseek(int fd, int position)
{
  // From fd, find the OFT entry. 
  if (fd < 0 || running->fd[fd] == NULL){
    printf("Not pointing at OFT entry\n");
    return -1; 
  }

  // change OFT entry's offset to position but make sure NOT to over run either end of the file.
  if (position > 0){
    if (position > running->fd[fd]->mptr->INODE.i_size){
      printf("Size overrun\n");
      return -1; 
    }
    int off = running->fd[fd]->offset;
    running->fd[fd]->offset = position;
    return off; 
  }

  return 1; // Eventually: return original position in file
}

int pfd()
{
  //display the currently opned files to help user know what files has been opened 
  printf("   fd     mode     count     offset     INODE\n");
  printf("  ----    ----     -----     -----     -------\n");
  for (int i=0; i < NFD; i++){
    if (running->fd[i]){
      OFT *cur = running->fd[i];
      char mode[8];
      switch(cur->mode){
        case 0:
          strcpy(mode, "READ"); 
          break;
        case 1:
          strcpy(mode, "WRITE");
          break;
        case 2:
          strcpy(mode, "R/W");
          break;
        case 3:
          strcpy(mode, "APPEND");
          break; 
      }
      // print fd, mode, count, offset, and INODE
      printf("    %d \t  %s      %d        %d        [%d, %d]\n", i, mode, cur->refCount, cur->offset, cur->mptr->dev, cur->mptr->ino);
      printf("  ----------------------------------------------\n");  
    }
  }
  return 0;
}

int dup(int fd)
{
  // verify fd is an opened descriptor;
  if (running->fd[fd] == NULL){
    printf("not OFT entry\n");
    return -1; 
  }
  // duplicates (copy) fd[fd] into FIRST empty fd[ ] slot;
  OFT *oftp = running->fd[fd];
  for (int i=0; i < NFD; i++){
    if (running->fd[i] == NULL){
      running->fd[i] = oftp;
      // increment OFT's refCount by 1;
      oftp->refCount++;
      return 0; 
    }
  }
  return -1;
}

int dup2(int fd, int gd)
{
  // CLOSE gd fisrt if it's already opened;
  if (running->fd[gd] != NULL){
    int r = close_file(gd);
    if (r == -1){
      printf("can't close file gd\n");
      return -1; 
    }
  }
  // duplicates fd[fd] into fd[gd];
  OFT *oftp = running->fd[fd];
  running->fd[gd] = oftp;
  // increment OFT's refCount by 1; 
  oftp->refCount++;

  return 1;
}

#endif