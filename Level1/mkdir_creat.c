/************* mkdir_creat.c file **************/

#ifndef __MKDIRCREAT_C__
#define __MKDIRCREAT_C__

// #include "type.h"
// #include "util.c"

//reference lecture book 11.8.1
int enter_name(MINODE *pip, int ino, char *name)
{
    char buf[BLKSIZE], *cp;
    int i, blk, block_i;   
    INODE *ip;  
    DIR *dp; 

    int need_length = 4* ((8 + (strlen(name)) + 3) / 4);   //a multiple of 4
    printf("need_len for %s = %d\n", name, need_length); 
    ip = &pip->INODE;   //get the inode 

    //for each data block of parent DIR do  //assume: only 12 direct blocks 
    for (int i=0; i < 12; i++){ //find empty block 
        if (ip->i_block[i] == 0)    //if no space in existing data block 
            break; 
        get_block(pip->dev, ip->i_block[i], buf);    //get empty block 
        block_i = i; 
        dp = (DIR *)buf; 
        cp = buf;
        blk = ip->i_block[i]; 
        printf("parent data blk[i] = %d\n", blk); 

        while (cp + dp->rec_len < buf + BLKSIZE){
            cp += dp->rec_len;
            dp = (DIR *)cp; 
        }   //dp NOW points at last entry in block 

        int ideal_length = 4* ((8 + dp->name_len +3) / 4);    //a multiple of 4 
        printf("\nideal_len = %d, rec_len = %d\n", ideal_length, dp->rec_len); 
        printf("trim [%d %s] rec_len to ideal_len %d\n", dp->rec_len, dp->name, ideal_length);

        int remain = dp->rec_len - ideal_length;    //last entry's rec_len - its ideal_length 
        
        if (remain >= need_length){
            //enter the new entry as the LAST entry and 
            //trim the previous entry rec_len to its ideal_length; 
            dp->rec_len = ideal_length; //trim last rec_len to ideal_len 
            cp += dp->rec_len;
            dp = (DIR *)cp; 
            dp->inode = ino; 
            dp->rec_len = remain; 
            dp->name_len = strlen(name); 
            strcpy(dp->name, name);
        }
    } 
    put_block(dev, blk, buf); 
    printf("writing parent data block %d to disk\n", blk); 
    return 0;
}

int my_mkdir(MINODE *pip, char *name)
{
    char buf[BLKSIZE], *cp; 
    DIR *dp; 
    int ino = ialloc(dev), blk = balloc(dev), i;    //allocate an INODE and a disk block; 
    printf("ialloc: ino=%d balloc: bno=%d \n", ino, blk);

    //creates an INODE=(dev, ino) in a minode, and weites the INODE to disk 
    MINODE *mip = iget(dev, ino);   //load INODE into a minode 
    INODE *ip = &mip->INODE;    //initialize mip->INODE as a DIR INODE 
    ip->i_mode = 0x41ED;    //040775: DIR type and permissions. set to dir type and set perms
    ip->i_uid = running->uid;   //owner uid 
    ip->i_gid = running->gid;   //group id
    ip->i_size = BLKSIZE;   //size in bytes 
    ip->i_links_count = 2;  //links count =2 because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L); //set to current time 
    ip->i_blocks = 2;   //LINUX: Blocks count in 512-byte chunks 
    ip->i_block[0] = blk;   //new DIR has one data block 
    //ip->i_block[1] to ip->i_block[14] = 0
    for (i=1; i <= 14; i++)
        ip->i_block[i] = 0;
    mip->dirty = 1; //mark minode dirty 
    iput(mip);  //write INODE to disk  

    //create data block for new DIR containing . and .. entries 
    bzero(buf, BLKSIZE);    //clear buf[] to 0;
    cp = buf;
    dp = (DIR *)cp; //write . to buf 
    //makr . entry 
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->name[0] = '.';
    //make .. entry: pino=parent DIR ino, blk=allocated block
    cp += dp->rec_len; 
    dp = (DIR *)cp; //move pointer to end of last entry into buf 
    dp->inode = pip->ino;
    dp->rec_len = BLKSIZE-12;   //rec_Len spans block 
    dp->name_len = 2;
    dp->name[0] = dp->name[1] = '.';
    put_block(dev, blk, buf);   //write to blk on diks 
    enter_name(pip, ino, name); 

    return 1;
}

int make_dir(char * pathname)
{
    MINODE *start; 
    char path1[256], path2[256]; 
    strcpy(path1, pathname);
    strcpy(path2, pathname);    //dirname/basename destroy pathname, making a copy 

    if (pathname[0] == '/'){
        start = root; 
        dev = root->dev; 
    }
    else {
        start = running->cwd; 
        dev = running->cwd->dev;
    }

    char *parent = dirname(path1);
    char *child = basename(path2);

    //dirname must exist and is a DIR
    MINODE *pino = getino(parent);  //get parent inode number 
    MINODE *pmip = iget(dev, pino); //get inode of parent 

    if (!S_ISDIR(pmip->INODE.i_mode)){  //check pmip->INODE is a dir 
        printf("%s is not directory\n", parent);
        return -1;
    }
    printf("check child NOT exist in parent directory\n"); 

    //basename must not exist in parent DIR 
    if (search(pmip, child)==0){    //if can't find child name in start MINODE 
        int r = my_mkdir(pmip, child);    //create a DIR 
        pmip->INODE.i_links_count++;    //increment link count    
        pmip->INODE.i_atime = time(0L); //touch atime 
        pmip->dirty = 1;    //make dirty 
        iput(pmip);    //write to disk 
        printf("enter new dir = %s\n", pathname); 
        return r; 
    }
    else{
        printf("Dir %s already exsists under parent %s", child, parent);
    }
    return 0;
}

int my_creat(MINODE *pip, char *name)
{
    char *buf[BLKSIZE], *cp; 
    DIR *dp;
    int ino = ialloc(dev), blk = balloc(dev);
    printf("ialloc: ino=%d balloc: bno=%d \n", ino, blk);

    MINODE *mip = iget(dev, ino);   //load INODE into a minode 
    INODE *ip = &mip->INODE;
    mip->INODE.i_mode = 0x81A4;   //file type and permissions 
    ip->i_uid = running->uid;   //owner uid
    ip->i_gid = running->gid;    //group ID
    ip->i_size = 0;   //size in bytes 
    ip->i_links_count = 1;  //link cpunt=1 
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L); //current time 
    ip->i_blocks = 2;   //LINUX: Blocks count in 512=byte chunks 
    ip->i_block[0] = 0; //new FILE has 0 data block 
    for (int i=1; i < 15; i++){ //clear block memory 
        ip->i_block[i] = 0;
    }
    mip->dirty = 1; //make minode dirty 
    iput(mip);  //write INODE to disk 
    enter_name(pip, ino, name);

    return 1;
}

int creat_file(char *pathname)
{
    MINODE *start; 
    if (pathname[0] == '/'){
        start = root; 
        dev = root->dev; 
    }
    else {
        start = running->cwd; 
        dev = running->cwd->dev;
    }

    char *parent = dirname(pathname);
    char *child = basename(pathname);

    //dirname must exist and is a DIR
    MINODE *pino = getino(parent);
    MINODE *pmip = iget(dev, pino); 

    if (!S_ISDIR(pmip->INODE.i_mode)){  //check pmip->INODE is a dir 
        printf("Access denied, %s is not directory\n", parent);
        return -1;
    }

    if (search(pmip, child)==0){
        int r = my_creat(pmip, child);    //create a DIR 
        pmip->INODE.i_atime = time(0L); //touch atime 
        pmip->dirty = 1;    //make dirty 
        iput(pmip);    //write to disk 
        printf("enter new file = %s\n", pathname); 
        return r; 
    }
    else{
        printf("Dir %s already exsists under parent %s", child, parent);
    }
    return 0;
}

#endif 

