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

    if( (running->fd[fd]->mode != 1) && (running->fd[fd]->mode != 2))
        printf("File is not open for write\n");
    return(my_write(fd, string, strlen(string)));
}

int my_write(int fd, char buf[], int nbytes)
{
    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->mptr;
    char dbuf[1024];
    int ibuf[256] = {0}, lbk = 0, startByte = 0, blk, remain, count = 0, dblk;
    char *cq = buf;
    char *cp;
    char wbuf[1024];

    while (nbytes > 0) 
    {
        lbk = oftp->offset / BLKSIZE;
        startByte = oftp->offset % BLKSIZE;

        if (lbk < 12) { // Direct blocks
            if (mip->INODE.i_block[lbk] == 0) 
            {
                mip->INODE.i_block[lbk] = balloc(mip->dev); // must allocate a block
            }
        blk = mip->INODE.i_block[lbk]; // blk should be a disk block now
        } 
        else if (lbk >= 12 && lbk < 256 + 12) // Indirect blocks
        {      
            if (mip->INODE.i_block[12] == 0) // indirect block hasn't been made yet
            {           
                mip->INODE.i_block[12] = balloc(mip->dev); // allocate a block
                memset(dbuf, 0, BLKSIZE); // zero out block on disk
                put_block(mip->dev, mip->INODE.i_block[12], dbuf);
                mip->INODE.i_blocks++; // increase # of blocks
            }

        // read data from block
        get_block(mip->dev, mip->INODE.i_block[12], (char *)ibuf);

        blk = ibuf[lbk - 12];
            if (blk == 0) 
            { // can't find a block. need to create one
                ibuf[lbk - 12] = balloc(mip->dev);
                put_block(mip->dev, mip->INODE.i_block[12], (char *)ibuf);
                blk = ibuf[lbk - 12];
            }
        } 
        else 
        { // double indrect blocks
            if (mip->INODE.i_block[13] == 0) 
            {
                mip->INODE.i_block[13] = balloc(mip->dev); // allocate a block
                memset(dbuf, 0, BLKSIZE); // zero out block on disk
                put_block(mip->dev, mip->INODE.i_block[13], dbuf);
            }

            get_block(mip->dev, mip->INODE.i_block[13], (char *)ibuf);
            lbk -= (256 + 12);
            int dblk = ibuf[lbk / 256]; // double indirect block

            if (dblk == 0) 
            {
                ibuf[lbk / 256] = balloc(mip->dev); // allocate a block
                memset(dbuf, 0, BLKSIZE); // zero out block on disk
                put_block(mip->dev, dblk, dbuf);
                put_block(mip->dev, mip->INODE.i_block[13], (char *)ibuf);
            }

            get_block(mip->dev, dblk, (char *)ibuf);
            blk = ibuf[lbk % 256]; // second block

            if (blk == 0) 
            {
                ibuf[lbk % 256] = balloc(mip->dev);
                put_block(mip->dev, dblk, (char *)ibuf);
                blk = ibuf[lbk % 256];
            }
        }
        // write to data block
        get_block(mip->dev, blk, wbuf); 
        cp = wbuf + startByte; // cp points at startByte in wbuf[]
        remain = BLKSIZE - startByte; // number of BYTEs remain in this block
        while (remain > 0){
            //*cp++ = *cq++;  //cq points at buf[]
            strcpy(cp, cq);
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
    //printf("wrote %d char into file descriptor fd=%d\n", count, fd);
    return nbytes; 
}

int my_cp(char *src, char *dest)
{
    int n = 0; 
    char buf[1024];
    //Open the source file as a read, open the target file as a write
    int fd = open_file(src, 0);   //open src for read
    int gd = open_file(dest, 1);  //open dst for WR|CREAT

    if (gd < 0){
        printf("creating file \n");
        creat_file(dest);
        gd = open_file(dest, 1); 
    }
    // open src for R and dest for W
    while (n = my_read(fd, buf, 1024)){
        buf[n] = 0;
        my_write(gd, buf, n);
        memset(buf, 0, 1024);
    }
    close_file(fd);
    close_file(gd);
    return 0;
}

#endif