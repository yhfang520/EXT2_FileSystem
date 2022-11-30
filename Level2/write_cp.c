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
    printf("enter text "); 
    scanf("%s", &string); 

    // check if fd is open for writing 
    printf("current mode = %d \n", running->fd[fd]->mode);

    if(running->fd[fd]->mode == 1 || running->fd[fd]->mode == 2)
        return (my_write(fd, string, strlen(string)));
    else 
        printf("File is not open for write\n");
    return -1; 
}

int my_write(int fd, char buf[], int nbytes)
{
    printf("mywrite test: %d %d\n", (int)strlen(buf), nbytes); 
    int count = nbytes, lbk, startByte, remain, blk;
    int ibuf[256]; 
    char wbuf[BLKSIZE];
    OFT *oftp;
    oftp = running->fd[fd];
    MINODE *mip;
    mip = oftp->mptr;
    INODE *ip = &mip->INODE;  
 
    while (nbytes > 0)
    {
        printf("%d\n",nbytes);
        // compute logical block (lbk) and startbyte in lbk
        lbk = oftp->offset / BLKSIZE; // get the logical block number 
        startByte = oftp->offset % BLKSIZE; // get the start byte in the block

        if (lbk < 12) 
        {
            printf("mywrite test, direct block\n"); 
            if (mip->INODE.i_block[lbk] == 0)
            {
                mip->INODE.i_block[lbk] = balloc(mip->dev); // allocate a block
            }
            blk = mip->INODE.i_block[lbk];
            printf("direct block done\n");
         
        }        
        else if (lbk >= 12 && lbk < 256 + 12)
        {
            printf("mywrite test, indirect block\n"); 
            // indirect blocks
            if (mip->INODE.i_block[12] == 0)
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
            printf("mywrite test, double indirect block\n"); 
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
        printf("mywrite test get data block blk into wbuf\n"); 
        memset(wbuf, 0, BLKSIZE);   //reset wbuf, not add too many characters in the last buffer 
        get_block(mip->dev, blk, wbuf);  //read disk block into wbuf[]
        char *cp = wbuf + startByte;    //cp points at startByte in wbuf[]
        int remain = BLKSIZE - startByte;   //number of BYTEs remain in this block 
        int *cq = buf; 
        //printf("%d\n",nbytes);
        while (remain > 0){ //write until remain == 0 
            //printf("%d\n",nbytes);
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
        //printf("%d\n",nbytes);
        put_block(mip->dev, blk, wbuf); //put wbuf into data block blk 
        // loop back to outer while to write more .... until nbytes are written    
    }
    printf("mywrite test make dirty\n");
    mip->dirty = 1; //mark mip dirty for iput()
    printf("wrote %d char into file descriptor fd=%d\n", count, fd);
    return nbytes; 
}

int my_cp(char *src, char *dest)
{
    int src_ino;
    int src_fd, dest_fd;
    char buf[BLKSIZE];
   
    // check if src exists
    src_ino = getino(src);
    if (src_ino == 0)
    {
        printf("Source file does not exist\n");
        return -1;
    }


    // open src for R and dest for W
    while (n = read(src_fd, buf, BLKSIZE)) // read from src
    {
        write(dest_fd, buf, n); // write n bytes from buf[ ] into fd
    }
    close(src_fd);
    close(dest_fd);
    return 0;
}

#endif