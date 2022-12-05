/************* mount_umount.c file **************/

#ifndef __MOUNTUMOUNT_C__
#define __MOUNTUMOUNT_C__


int mount(char *diskname, char *dest)
{
  int i, dev;
  MINODE *mip;
  INODE *ino;
  char buf[BLKSIZE];
  GD *gp;
  SUPER *sp;
  MTABLE *mp; 
  

  // check if disk is already mounted else allocate a free mount table entry 
  for (i=0; i<NMTABLE; i++)
  {
    if(!strcmp(mtable[i].devName, diskname)){
      printf("disk is already mounted\n");
      return -1;
    }
  }
  // allocate a free MOUNT table entry 
  for (i=0; i<NMTABLE; i++){
    if(mtable[i].dev != 0)
    {
      mp = &mtable[i];
      break;
    }
  }
  mp = &mtable[i];
  strcpy(mp->devName, diskname);
  strcpy(mp->mntName, dest);

  // open disk for RW
  dev = open(diskname, O_RDWR);
  // make sure is open 
  // check its an EXT2 file system 
  get_block(dev,1,buf);
  sp = (SUPER *)buf;
  if(sp->s_magic != 0xEF53)
  {
    printf("not an EXT2 file system\n");
    return -1;
  }
  // get the ino and minode
  ino = getino(diskname); 
  mip = iget(running->cwd->dev, ino);
  // verify mount_point is DIR
  if (!S_ISDIR(mip->INODE.i_mode)){
     printf("WARNING: %s is not a directory\n", diskname);
     return -1; 
  }
  // check mount point is not busy 
  if(mip->refCount > 1)
  {
    printf("mount point is busy\n");
    return -1;
  }
  // record new dev, ninodes, nblocks, bmap, imap, iblk in MOUNT table
  mp->dev = dev;
  mp->ninodes = sp->s_inodes_count;
  mp->nblocks = sp->s_blocks_count;
  mp->bmap = gp->bg_block_bitmap;
  mp->imap = gp->bg_inode_bitmap;
  mp->iblock = gp->bg_inode_table;

  // mare the mount point minode as mpounted on and let it point at the moount table 
  mip->mounted = 1;
  mip->mptr = mp;
  return 0;
}

int umount(char *pathname)
{
  MTABLE *mp;
  int i;
  
  //search the mount table to check filesys is indeed mounted 
   for (i = 0; i < NMTABLE; i++){
    if (strcmp(mtable[i].devName, pathname) == 0){
      mp = &mtable[i];
      break; 
    }
  }
  
  //check whether any file is still active in the mounted filesys;
  for (i = 0; i < NMINODE; i++){
    if (minode[i].dev == mp->dev){  //check if dev in minodes macthes the dev number of the mounted file system
      printf("File %s is active\n", pathname);
      return -1; 
    }
  }

  //find mount_point in-memory inode  
  MINODE *mip = mp->mntDirPtr;
  mip->mounted = 0; //reset the minode's mounted flag to 0 
  iput(mip);  

  return 0;
}

#endif