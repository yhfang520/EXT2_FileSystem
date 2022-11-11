/************* link_unlink.c file **************/

#ifndef __LINKUNLINK_C__
#define __LINKUNLINK_C__

#include "mkdir_creat.c"
#include "rmdir.c"

//11.8.6
int my_link(char *pathname, char *new_file)
{
    char parent[256], child[256]; 

    if (pathname[0] == '/'){
        dev = root->dev; 
    }else {
        dev = running->cwd->dev; 
    }

    //verify old_file exists and is not a DIR 
    int oino = getino(pathname);
    MINODE *omip = iget(dev, oino);
    printf("my_link test: parent ino %d\n", oino);

    //check omip->INODE file type (must not be DIR)
    if(S_ISDIR(omip->INODE.i_mode)){
        printf("klink: %s is a DIR, can't link to DIR\n", pathname); 
        return -1; 
    }

    //new_file must not exist yet 
    int new = getino(new_file); 
    if (new != -1){
        printf("file exist\n");
        return -1;
    } 

    //creat new_file with the same inode number of old_file;
    strcpy(parent, dirname(new_file));
    strcpy(child, basename(new_file));
    int pino = getino(parent);
    MINODE *pmip = iget(dev, pino);
   
    //creat entry in new parent DIR with same inode number of old_file
    enter_name(pmip, oino, child);
    omip->INODE.i_links_count + 1;    //inc INODE's links_count by 1 //this is where the fix is 
    omip->dirty = 1;    //for write back by iput(omip)
    iput(omip);
    iput(pmip);
    printf("------- verify link results -------\n");
    findmyname(pmip, oino, pathname);
    return 1;
}

int my_unlink(char *pathname)
{
    char parent[256], child[256];

    //get filename's minode 
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino); 

    //check it's a REG or symbolic LNK file; can't not be a DIR 
    if (S_ISDIR(mip->INODE.i_mode)){
        printf("can't unlink DIR file\n");
        return -1; 
    }

    //remove name entry from parent DIR's data block 
    strcpy(parent, dirname(pathname));
    strcpy(child, basename(pathname));
    int pino = getino(parent);
    MINODE *pmip = iget(dev, pino);
    printf("call rm_child [%d %d] %s \n", mip->dev, pino, pathname); 
    rm_child(pmip, child);
    pmip->dirty = 1;
    iput(pmip);

    //decrement INODE's link_count by 1
    mip->INODE.i_links_count--;
    if (mip->INODE.i_links_count > 0){
        mip->dirty = 1; //for write INODE back to disk 
    } else {
        //deallocate all data blocks in INODE; 
        for (int i=0; i < 12 && mip->INODE.i_block[i] !=0; i++){ 
            bdalloc(dev, mip->INODE.i_block[i]);
        }
        idalloc(dev, ino); 
    }
    printf("unlink %s\n", pathname); 
    printf("------- verify unlink results -------\n");
    findmyname(pmip, ino, pathname);
    iput(mip);  //release mip 
    return 1;
}

int my_rm(MINODE *mip, char *pathname)
{
    return 1;
}

#endif