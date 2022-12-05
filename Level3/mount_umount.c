/************* mount_umount.c file **************/

#ifndef __MOUNTUMOUNT_C__
#define __MOUNTUMOUNT_C__


int mount(char *diskname, char *dest)
{
  int i, dev, ino;
  MINODE *mip;
  char buf[BLKSIZE];
  GD *gp;
  SUPER *sp;
  MTABLE *mp; 

  // check if disk is already mounted
  for (i=0; i<NMTABLE; i++){
    mp = &mtable[i];
    if (mp->dev == dev){
      printf("disk %s is already mounted\n", diskname);
      return -1;
    }
  }
  // allocate a free MOUNT table entry 
  for (i=0; i<NMTABLE; i++){
    mp = &mtable[i];
    if (mp->dev == 0){
      break;
    }
  }
  return 1;
}

int umount(char *pathname)
{
  return 0;
}

#endif