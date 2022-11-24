/************* write_cp.c file **************/

#ifndef __WRITECP_C__
#define __WRITECP_C__

#include "read_cat.c"
#include "../Level1/link_unlink.c"

int write_file()
{
    int fd = 0;
    char string[BLKSIZE]; 
    pfd();
    printf("enter fd = \n");
    scanf("%d", &fd);
    printf("enter text \n"); 
    scanf("%s", string); 
    
    // check if fd is open for writing
    if(running->fd[fd]->mode != 1 || running->fd[fd]->mode != 2){
        printf("File is not open for write\n");
        return -1;
    }
    my_write(fd, string, sizeof(string));
    return 1; 
}

int my_write(int fd, char buf[], int nbytes)
{
    int count = 0, lbk, startByte, remain, blk;
    int ibuf[256]; 
    char wbuf[BLKSIZE];
    OFT *oftp;
    oftp = running->fd[fd];
    MINODE *mip;
    mip = oftp->mptr; 
    while (nbytes > 0)
    {
        // compute logical block (lbk) and startbyte in lbk
        lbk = oftp->offset / BLKSIZE; // get the logical block number 
        startByte = oftp->offset % BLKSIZE; // get the start byte in the block

        if (lbk < 12) 
        {
            if (ip->i_block[lbk] == 0)
            {
                mip->INODE.i_block[lbk] = balloc(mip->dev); // allocate a block
            }
            blk = mip->INODE.i_block[lbk];
        }        
        else if (lbk >= 12 && lbk < 256 + 12)
        {
          
            // indirect blocks
            if (ip->i_block[12] == 0)
            {
                if (mip->INODE.i_block[12] == 0)    //no data block 
                {
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
            }
        }
        else    //double indirect blcoks 
        {
            lbk -= (12 + 256);
            if (mip->INODE.i_block[13] == 0)    //if accesss pointer is empty, allocate a block and save its pointer 
            {
                mip->INODE.i_block[13] = balloc(mip->dev);  //allocate a block 
                get_block(mip->dev, mip->INODE.i_block[13], ibuf);   //get block into memory 
                bzero(buf, BLKSIZE); // clear the block 
            }
            get_block(mip->dev, mip->INODE.i_block[13], ibuf); // get the block into memory 
            int i = lbk / 256; // integer of the indirect block
            int j = lbk % 256;
        } 
        get_block(mip->dev, blk,wbuf);  //read disk block into wbuf[]
        char *cp = wbuf + startByte;    //cp points at startByte in wbuf[]
        int remain = BLKSIZE - startByte;   //number of BYTEs remain in this block 
        int *cq = buf; 
        while (remain > 0){ //write until remain == 0 
        
            // change this later to write more than 1 byte at a time
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
    printf("wrote %d char into file descriptor fd=%d\n", nbytes, fd);
    return nbytes; 
}

int my_cp(char *pathname)
{
    return 1;
}

#endif