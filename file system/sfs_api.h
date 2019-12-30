//
//  sfs.h
//  
//
//  Created by kristen 
//

#ifndef sfs_h
#define sfs_h

#include <stdio.h>
#include <stdint.h>

#define MAX_fname 21


#define MAXFILENAME 20
//16+1(.)+3(extension)+1(\0)

#define NUM_BLOCK 10240;

void force_set_index(uint32_t index);
int get_index();
void rm_index(uint32_t index);


typedef struct superblock_tlb {
    uint32_t magic;
    uint32_t block_size;
    uint32_t fs_size;
    uint32_t iNode_tlb_size;
    uint32_t root_dirctory_iNode;
} superblock_tlb;

typedef struct inode_t{
    uint32_t mode;
    uint32_t link_cnt;
    uint32_t uid;
    uint32_t gid;
    int  size;
    int data_block_ptr[12];
    int ind_ptr;
}inode_t;

typedef struct file_descriptor{
    int32_t iNodeIndex;
    //assume the iNode tbale has a size of 2^23*18*4
	inode_t* inode;
    int read_ptr;
    int write_ptr;
}file_descriptor;


typedef struct directory_entry {
    int num;
    char name[MAX_fname];
}directory_entry;


void mksfs(int fresh);
int sfs_getnextfilename(char *fname);
int sfs_getfilesize(const char* path);
int sfs_fopen(char *name);
int sfs_fclose(int fileID);
int sfs_frseek(int fileID, int loc);
int sfs_fwseek(int fileID, int loc);
int sfs_fwrite(int fileID, char *buf, int loc);
int sfs_fread(int fileID, char *buf, int loc);
int sfs_remove(char *file);


#endif //sfs_h
