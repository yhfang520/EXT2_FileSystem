/************* rmdir.c file **************/

#ifndef __RMDIR_C__
#define __RMDIR_C__

#include "../Level2/open_close.c"

//11.8.4
int rm_child(MINODE *pip, char *name)
{
  char buf[BLKSIZE], *cp, temp[256], *rm_cp;
  int i, j, block_i, last_len, size, rm_len; 
  DIR *dp;

  for (i=0; i < pip->INODE.i_blocks; i++){ //search DIR direct blocks only 
    if (pip->INODE.i_block[i] == 0)
      return 0;

    //last entry in the data block 
    get_block(pip->dev, pip->INODE.i_block[i], buf);
    // printf("rm child test i=%d name=%s\n", i, name);
    dp = (DIR *)buf; 
    cp = buf;
    block_i=i;
    i = 0;  
    j = 0; 
    while (cp + dp->rec_len < buf + BLKSIZE){
      strncpy(temp, dp->name, dp->name_len);  //make name a string 
      temp[dp->name_len] = 0; //in temp[]
    
      if (!strcmp(name, temp)){
        i = j; 
        rm_cp = cp; 
        rm_len = dp->rec_len;
      } 
      last_len = dp->rec_len; 
      cp += dp->rec_len;  //advance cp by rec_len  
      dp = (DIR *)cp;
      j++;  //get count of entries inro j 
    } //dp NOW points at last entry in block 

    printf("previous = [%d %s]", last_len, temp);
    strncpy(temp, dp->name, dp->name_len);  //make name a string 
    temp[dp->name_len] = 0; //in temp[]
    printf("LAST entry = [%d %s]\n", dp->rec_len, pathname); 

    if (j == 0){ //first and only entry in a data block 
      bdalloc(pip->dev, pip->INODE.i_block[block_i]); //deallocate its data blocks and inode 
      for (i=block_i; i < pip->INODE.i_blocks; i++) {//move other blocks up 
        printf("move other blocks up\n"); 
      }
    } else if (i == 0){ //last entry in block 
      cp -= last_len;
      last_len = dp->rec_len;
      dp = (DIR *)cp; 
      dp->rec_len += last_len;
    } else {  //entry is first but not the only entry or in the middle of a block
      size = buf+BLKSIZE - (rm_cp + rm_len);  //in middle, copying size bytes
      memmove(rm_cp, rm_cp + rm_len, size); //move all trailing entries LEFT to overlay the delted entry; 
      cp -= rm_len;
      dp = (DIR *)cp; 
      dp->rec_len += rm_len;
      strncpy(temp, dp->name, dp->name_len);  //make name a string
      temp[dp->name_len] = 0; //in temp[]
    }
    put_block(pip->dev, pip->INODE.i_block[block_i], buf);
  } 
  return 0; 
}

int remove_dir(char *pathname)
{ 
  MINODE *mip, *pmip; 
  DIR *dp;
  char temp[256], buf[BLKSIZE], name[256], *cp;
  //get in-memory INODE of pathname 
  strcpy(temp, pathname);
  int ino = getino(pathname);
  int i, pino; 
  if (ino == -1){
      printf("ino does not exist\n");
      return 0; 
  }
  mip = iget(dev, ino); 
  findmyname(mip, ino, name); //find name from parent DIR 
  printf("check dir is empty by step through its data block\n");
  
  if (running->uid != mip->INODE.i_uid || running->uid !=0){  //check running PROC is user 
    return 0; 
  }

  if (!S_ISDIR(mip->INODE.i_mode)){
    printf("its not a dir\n");
    return -1; 
  }

  get_block(dev, mip->INODE.i_block[0], buf);
  dp = (DIR*)buf;
  cp = buf;
  
  int item = 0;
  while (cp < buf + BLKSIZE)
  {
    item +=1;
    cp += dp->rec_len;
    dp = (DIR*) cp;

  }
  
  if (item != 2)
  {
    printf("cannot rmdir: %s is not empty\n",pathname);
    printf("%d\n", item);
    return -1;
  }

  // if (mip->refCount > 2){//check if its unused 
  //   printf("node used, refCount=%d\n", mip->refCount);
  //   return -1; 
  // }

  //verify DIR is empty (traverse data blocks for number of entries = 2);
  if (mip->INODE.i_links_count <= 2){ //check reg files 
    int actual_link = 0;
    get_block(dev, mip->INODE.i_block[0], buf);
    dp = (DIR *)buf; 
    cp = buf; 

    while(cp < buf + BLKSIZE){
      actual_link++;
      cp += dp->rec_len; 
      dp = (DIR *)cp; 
    }

    if (actual_link <= 2){  
      for (i=0; i < 12; i++){
        if (mip->INODE.i_block[i]==0)
          continue;
        else 
          bdalloc(mip->dev, mip->INODE.i_block[i]); //dealloc mip blocks 
        }
        printf("deallocate data blcok# %d\n", mip->INODE.i_block[0]);
        idalloc(mip->dev, mip->ino);  //dealloc mip inode 
        printf("deallocate inode# %d\n", mip->ino); 
        iput(mip); 
        //get parent's ino and inode 
        u32 *inum = malloc(8); 
        pino = findino(mip, inum); //get pino from .. entry in INODE.i_block[0]
        pmip = iget(mip->dev, pino); 
        //get name from parent DIR's data block 
        findmyname(pmip, ino, name);  //find name from parent DIR 
        // printf("remove_dir test: pino %d, ino %d, name %s\n", pino, ino, name); 

        if (strcmp(name, ".") !=0 && strcmp(name, "..") != 0 && strcmp(name, "/") != 0){
          // printf("remove dir test call rm_child\n"); 
          //remove name from parent directory 
          rm_child(pmip, name);
          pmip->INODE.i_links_count--;  //dec link count 
          pmip->INODE.i_atime=pmip->INODE.i_mtime=time(0L);             
          pmip->dirty=1;  //make dirty 
          //deallocate its data blocks and inode 
          bdalloc(pmip->dev, mip->INODE.i_block[0]);
          idalloc(pmip->dev, mip->ino);
          //dec parent links_count by 1; mark parent pimp dirty;
          iput(pmip);
        }
        printf("%s has removed\n", name); 
        printf("------- verify rmdir results -------\n");
        findmyname(pmip, ino, name);
    } else {
      printf("DIR is not empty\n");
    }
  }else{
    printf("DIR is not empty\n"); 
  }
  return 0;
}

#endif