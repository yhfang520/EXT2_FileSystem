/************* write_cp.c file **************/

#ifndef __WRITECP_C__
#define __WRITECP_C__

#include "read_cat.c"
#include "../Level1/link_unlink.c"

int write_file()
{
    int fd;
    char string[BLKSIZE] = {0}; 
    pfd();
    printf("enter fd = ");
    scanf("%d", &fd);
    printf("enter text : "); 
    scanf("%s", string); 

    // check if fd is open for writing 
    printf("current mode = %d \n", running->fd[fd]->mode);

    if(running->fd[fd]->mode != 1 || running->fd[fd]->mode != 2)
        printf("File is not open for write\n");
    return(my_write(fd, string, strlen(string)));
}

int my_write(int fd, char buf[], int nbytes)
{
    int count = nbytes, lbk, startByte, remain, blk, iblk, dblk;
    int ibuf[256], dbuf[256];
    char wbuf[1024];
    OFT *oftp;
    oftp = running->fd[fd];
    MINODE *mip;
    mip = oftp->mptr;
    char *cq = buf; 
    
    printf("nbytes = %d\n", nbytes);
    while (nbytes > 0){
        // compute logical block (lbk) and startbyte in lbk
        lbk = oftp->offset / BLKSIZE; // get the logical block number 
        startByte = oftp->offset % BLKSIZE; // get the start byte in the block
        if (lbk < 12){
            if (mip->INODE.i_block[lbk] == 0){
                mip->INODE.i_block[lbk] = balloc(mip->dev); // allocate a block
            }
            blk = mip->INODE.i_block[lbk];
            printf("allocate direct block blk=%d\n", blk); 
        }  
        else if (lbk >= 12 && lbk < 256 + 12){
            // indirect blocks
            if (mip->INODE.i_block[12] == 0){
                if (mip->INODE.i_block[12] == 0){    //no data block 
                    mip->INODE.i_block[12] = balloc(mip->dev);  //allocate a block 
                    get_block(mip->dev, mip->INODE.i_block[12], ibuf);   //get block into memory 
                    bzero(buf, BLKSIZE); // clear the block 
                }
                // clear all blk entries to 0
                get_block(mip->dev, mip->INODE.i_block[12], ibuf); // get the block into memory 
                blk = ibuf[lbk - 12];
                if (blk == 0){
                    blk = balloc(mip->dev); //allocate a disk block; 
                    put_block(mip->dev, mip->INODE.i_block[12], ibuf);  //record in i_block[12]
                }
                printf("allocate indirect block blk=%d\n", blk);
            }
        }
        else{    //double indirect blcoks 
            iblk = (lbk - (12 + 256) / 256);
            dblk = (lbk - (23 + 256) % 256);
            if (mip->INODE.i_block[13] == 0){    //if accesss pointer is empty, allocate a block and save its pointer 
                mip->INODE.i_block[13] = balloc(mip->dev);  //allocate a block 
                get_block(mip->dev, mip->INODE.i_block[13], ibuf);   //get block into memory 
                bzero(ibuf, 256); // clear the block 
            }
            get_block(mip->dev, mip->INODE.i_block[13], ibuf); // get the block into memory 
            if (ibuf[iblk] == 0){
                ibuf[iblk] = balloc(mip->dev);
                put_block(mip->dev, mip->INODE.i_block[13], (char *)ibuf);
                get_block(mip->dev, ibuf[iblk], (char *)dbuf);
                bzero(dbuf, 256);
                put_block(mip->dev, ibuf[iblk], (char *)dbuf);
            }
            get_block(mip->dev, ibuf[iblk], (char *)dbuf);
            if (dbuf[dblk] == 0){
                dbuf[dblk] = balloc(mip->dev);
                put_block(mip->dev, ibuf[iblk], (char *)dbuf);
            }
            blk = dbuf[dblk];
            printf("allocate double indirect block blk=%d\n", blk);
        } 
        get_block(mip->dev, blk, wbuf);  //read disk block into wbuf[]
        char *cp = wbuf + startByte;    //cp points at startByte in wbuf[]
        int remain = BLKSIZE - startByte;   //number of BYTEs remain in this block 
        while (remain > 0){
            *cp++ = *cq++;  //cq points at buf[]
            nbytes--; remain--; //dec counts
            oftp->offset++; //dec counts 
            //if offset is greater than size increase file size 
            if (oftp->offset > mip->INODE.i_size){ //especially for RW|APPEND mode
                mip->INODE.i_size++;
            }    
            //break when we have read all the bytes 
            if (nbytes <= 0)
                break; 
        } 
        put_block(mip->dev, blk, wbuf); //put wbuf into data block blk 
        // loop back to outer while to write more .... until nbytes are written    
    }
    mip->dirty = 1; //mark mip dirty for iput()
    printf("wrote %d char into file descriptor fd=%d\n", count, fd);
    return nbytes; 
}

int my_cp(char *src, char *dest)
{
    int n = 0; 
    char buf[1024];
    //Open the source file as a read, open the target file as a write
    int fd = open_file(src, 0);   //open src for read
    int gd = open_file(dest, 1);  //open dst for WR|CREAT

    // buf[1024] = 0;
    pfd();
    if (gd < 0){
        printf("creating file \n");
        creat_file(dest);
        gd = open_file(dest, "1"); 
    }
    // open src for R and dest for W
    while (n = read_file(fd, buf, 1024)){ // read from src
        //buf[n] = 0;
        my_write(gd, buf, n); // write n bytes from buf[ ] into fd
    }
    close_file(fd);
    close_file(gd);
    return 0;
}

#endif