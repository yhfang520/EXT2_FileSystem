/************* open_close_lseek.c file **************/

#ifndef __OPENCLOSELSEEK_C__
#define __OPENCLOSELSEEK_C__

#include "../Level1/mkdir_creat.c"

int truncate(MINODE *mip)
{
  INODE *ip = &mip->INODE; 
  int i;
  //iterate through all 12 direct blocks and free them
  for (i=0; i < ip->i_blocks; i++){
    if (ip->i_block[i] != 0)
      bzero(ip->i_block, BLKSIZE);  //be zero if it's not empty 
    else
      break; 
  }
  //iterate through 256 indirect blocks and free them

  //iterate through 256 double indirect blocks and free them

  //update INODE's time field
  ip->i_atime = ip->i_mtime = time(0L);
  //set INODE's size to 0 and mark Minode[ ] dirty
  ip->i_size = 0;
  mip->dirty = 1; 

  return 1;
}

//11.9.1
int open_file(char *pathname, int mode)
{
  int ino, i, pino, r;
  MINODE *mip, *pmip;
  // OFT *open; not being used 

  if (pathname[0] == '/')
    dev = root->dev;
  else 
    dev = running->cwd->dev;
  
  //get pathname's inumber, minode pointer 
  ino = getino(pathname);
  if (ino == -1){  //if file does not exist 
    char parent[BLKSIZE], buf[BLKSIZE];
    strcpy(buf, pathname);
    strcpy(parent, dirname(buf));
    pino = getino(parent);
    if (pino == -1)
      return -1;
    pmip = iget(dev, ino);
    r = my_creat(pmip, pathname); //creat it first, then 
    ino = getino(pathname);  //get its ino 
    if (ino == -1)
      return -1; 
  }
  mip = iget(dev, ino);

  //check mip->INODE.i_mode to verify it's REGULAR file and permission OK.
  if (!S_ISREG(mip->INODE.i_mode)){
    printf("WARNING: %s is not a regular file\n", pathname);
    return -1; 
  }

  if (!(mip->INODE.i_uid == running->uid || running->uid))
    return -1; 
  
  if (!(mip->INODE.i_gid == running->gid || running->gid))
    return -1; 

  //check whether the file is ALREADY opened with INCOMPATABLE mode 
  for (i=0; i < NFD; i++){
    if (running->fd[i] != NULL && running->fd[i]->mptr == mip){
      if (mode != 0){
        printf("%s is already opened with incompatable mode\n", pathname);
        return -1;
      }
    } 
  }

  //alloctae a FREE OpenFileTable (OFT) and fill in values 
  OFT *oftp = (OFT *)malloc(sizeof(OFT)); //build the open fd 
  oftp->mode = mode;  //mode = 0|1|2|3 for R|W|RW|APPEND
  oftp->refCount = 1;
  oftp->mptr = mip;  //point at the file's minode[]

  printf("%s opend in ", pathname);
  //Depending on the open mode 0|1|2|3, set the OFT's offset accordingly
  switch (mode){  //offset
    case 0: //Read: offset=0
      oftp->offset = 0;
      printf("read");
      break; 
    case 1: //Write: truncate file to 0 sizes 
      truncate(mip);
      oftp->offset = 0;
      printf("write");
      break; 
    case 2: //Read/Write: do NOT truncate file, offset = 0
      oftp->offset = 0;
      break; 
    case 3: //Append: offset to size of file 
      oftp->offset = mip->INODE.i_size; //APPEND mode  
    default:
      printf("invalid mode\n");
      return -1; 
  }

  printf(" mode = %d: fd = %d\n", mode, running->fd[i]);
  
  //find the SMALLEST i in running PROC's fd[ ] such that fd[i] is NULL
  for (i=0; i < NFD; i++){  //find empty fd in running PROC's fd array 
    if (running->fd[i] == NULL){
      running->fd[i] = oftp;  //assign the OFT to the fd[i] this was assigned to open
      break; 
    }
  }
  
  //update INODE's time filed 
  mip->INODE.i_atime = time(0L);  //update inode access time 
  if (mode != 0){ //if not R touch atime and mtime 
    mip->INODE.i_mtime = time(0L);
  }
  mip->dirty = 1;
  iput(mip);
  printf("%s opened in mode %d\n", pathname, mode);
  return i; // Eventually: return file descriptor of opened file
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
  printf("close: refCount = %d", oftp->refCount--); 
  printf("fd = %d is closed\n", running->fd[fd]); 
  iput(mip);  //release minode 
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
  printf("   fd     mode     offset     INODE\n");
  printf("  ----    ----     -----     -------\n");
  for (int i=0; i < NFD; i++){
    if (running->fd[i] == NULL)
      break;
    printf("  %d   %s   %d   [%d,%d]\n", i, running->fd[i]->mode, running->fd[i]->offset, running->fd[i]->mptr->dev, running->fd[i]->mptr->ino);
  }
  printf("  --------------------------------------\n"); 
  return 1;
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