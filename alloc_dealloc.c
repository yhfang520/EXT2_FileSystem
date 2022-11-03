/*********** alloc.c file ****************/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;
MTABLE  mtable[NMTABLE];

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd, dev, rootdev;
extern int nblocks, ninodes, bmap, imap, iblk;
extern char line[128], cmd[32], pathname[128];
/**** globals defined in main.c file ****/

int tst_bit(char *buf, int bit) //11.3.1, p.333
{
  return buf[bit/8] & (1 << (bit % 8));
}

int set_bit(char *buf, int bit)
{
  return buf[bit/8] |= (1 << (bit % 8));
}

int decFreeInodes(int dev)
{
  char buf[BLKSIZE];

  //dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf);
  get_block(dev, 2, buf); 
  gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(dev, 2, buf); 
}

int ialloc(int dev) //allocate an inode number from inode_bitmap 
{
  int i;
  char buf[BLKSIZE];

  //read inode_bitmap block 
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){  //use ninodes from SUPER block 
    if (tst_bit(buf, i) == 0){
      set_bit(buf, i);
      put_block(dev, imap, buf);
      //update free inode count in SUPER and GD
      decFreeInodes(dev);

      // printf("allocated ino = %d\n", i+1);  //bits count from 0; ino from 1
      return (i+1); 
    }
  }
  return 0; 
}

int decFreeBlocks(int dev)
{
  char buf[BLKSIZE];

  //dec free blocks count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count--;
  put_block(dev, 1, buf);
  get_block(dev, 2, buf); 
  gp = (GD *)buf;
  gp->bg_free_blocks_count--;
  put_block(dev, 2, buf); 
}

int balloc (int dev)
{
  //balloc(dev)-allocates a FREE disk block number from a device  
  int i; 
  char buf[BLKSIZE];

  //read block_bitmap block 
  get_block(dev, bmap, buf);

  for (i=0; i < nblocks ; i++){  //use nblocks from SUPER block 
    if (tst_bit(buf, i) == 0){
      set_bit(buf, i);
      put_block(dev, bmap, buf);
      //update free inode count in SUPER and GD
      decFreeBlocks(dev);

      // printf("disk block = %d\n", i+1);  //bits count from 0; ino from 1
      return (i+1); 
    }
  }
  return 0; 
}

int clr_bit(char *buf, int bit)
{
  //clear bit in char buf[BLKSIZE]
  buf[bit/8] &= ~(1 << (bit%8));
}

int incFreeInodes(int dev)
{
  char buf[BLKSIZE];
  //inc free inodes count in SUPER and GD 
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count++;
  put_block(dev, 1, buf);
  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count++;
  put_block(dev, 2, buf); 
}

int idalloc(int dev, int ino)
{
  int i;
  char buf[BLKSIZE];

  if (ino > ninodes){ //ninodes global 
    printf("inumber %d out od range\n", ino); 
    return 0; 
  }
  //get inode bitmap block 
  get_block(dev, imap, buf); 
  clr_bit(buf, ino-1); 
  //write buf back 
  put_block(dev, imap, buf);
  //update free inode count in SUPER and GD
  //printf("deallocate inode# %d\n", ino-1); 
  incFreeInodes(dev); 
}

int incFreeBlocks(int dev)
{
  char buf[BLKSIZE];
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count++;  //dec the super table
  put_block(dev, 1, buf);
  
  get_block(dev, 2, buf);
  gp = (GD *)buf; //dec the GD table 
  gp->bg_free_blocks_count++;
  put_block(dev, 2, buf);
  return 0; 
}

int bdalloc(int dev, int bno)
{
  //deallocates a disk block(number) bno 
  char buf[BLKSIZE];
  if (bno > nblocks){
    return 0; 
  }
  get_block(dev, bmap, buf);  //get block 
  clr_bit(buf, bno-1);  //clear bits to 0
  put_block(dev, bmap, buf);  //write block back 
  incFreeBlocks(dev); 
  //printf("deallocate data block# %d\n", bno-1);
  return 0;
}