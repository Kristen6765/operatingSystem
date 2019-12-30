//
//  sfs_api.c
//  
//
//  Created by Kristen


#include <stdbool.h> 
#include "sfs_api.h"
#include "disk_emu.h"
#include<string.h>

#define NUM_BLOCKS 10240
#define BIT_MAP_SIZE 1024 //1 byte represents a data block
#define BLOCK_SIZE 1024
#define NUM_INODES 100 //there are 100 inodes and each one is corresponding to one file
#define NUM_OF_FILES 100

superblock_tlb super_block;
file_descriptor file_descriptors[NUM_OF_FILES];
directory_entry directory_entry_tbl[NUM_OF_FILES];
inode_t inode_tbl[NUM_INODES];
int cur_file_index=0;
int inode_blocks=sizeof(inode_tbl)/BLOCK_SIZE+1;
int dir_block_num = sizeof(directory_entry) * NUM_OF_FILES / BLOCK_SIZE + 1;
char free_bit_map[NUM_BLOCKS];





void initialize(){
	
    //initialize super block
    super_block.magic=0xACBD0005;
    super_block.block_size=BLOCK_SIZE;
    super_block.fs_size=BLOCK_SIZE*NUM_BLOCKS;
    super_block.iNode_tlb_size=NUM_INODES;
    super_block.root_dirctory_iNode=0;
    inode_tbl[0].size=0;//
    
    
     //initialize inode_table
    for (int i = 0; i < NUM_INODES; i++) {
           inode_tbl[i].mode = 0;
           inode_tbl[i].link_cnt = 0;
           inode_tbl[i].uid = 0;
           inode_tbl[i].gid = 0;
           inode_tbl[i].size = -1;
           for (int j = 0; j < 12; j++) {
               inode_tbl[i].data_block_ptr[j] = -1;
           }
           inode_tbl[i].ind_ptr = -1;
       }
    
    //initialzie dircrory entry table
    for (int i=0; i<NUM_OF_FILES; i++){
               directory_entry_tbl[i].num = -1;
               for (int j = 0; j < MAX_fname; j++) {
                   directory_entry_tbl[i].name[j] = '\0';
           }
       }
}







//initialize file descriptor
void initialize_file_descriptor(){
    for (int i = 0; i < NUM_OF_FILES; i++) {
        file_descriptors[i].iNodeIndex = -1;
        file_descriptors[i].inode = NULL;
        file_descriptors[i].read_ptr = 0;
        file_descriptors[i].write_ptr = 0;
    }

    
}

/**
 * update disk
 */
void update_disk() {
    //write_blocks(int start_address, int nblocks, void *buffer)
    write_blocks(1, inode_blocks, &inode_tbl);
    write_blocks(1023, 1, free_bit_map);
    write_blocks(inode_blocks + 1, dir_block_num, &directory_entry_tbl);
    
}

/**
 * create file system
 * @param fresh
 */
void mksfs(int fresh){
    memset(free_bit_map, 0xff, sizeof(free_bit_map));
    //if !=0, then flag is true
    if( fresh){
        initialize();
		initialize_file_descriptor();
		memset(free_bit_map, 1, sizeof(free_bit_map));
		init_fresh_disk("disk.img", BLOCK_SIZE, NUM_BLOCKS);

    //write super bloc on disk
    write_blocks(0, 1, &super_block);
	get_index();

    //write inode table on disk
    for (int i = 0; i < inode_blocks; i++) {
        get_index();
    }
    write_blocks(1, inode_blocks, &inode_tbl);
        
    //write root dirctory on disk
        inode_tbl[0].size = sizeof(directory_entry) * NUM_OF_FILES;
        for (int i = 0; i < dir_block_num; i++) {
            int num = get_index();
            inode_tbl[0].data_block_ptr[i] = num;
        }
        write_blocks(inode_blocks + 1, dir_block_num, &directory_entry_tbl);
        
    //write free bit map on disk
    write_blocks(1023, 1, free_bit_map);
        
        
    }else {// flag is false, need to open form disk
        
        // read super block from disk
            read_blocks(0, 1, &super_block);
            // read the directory entry table
            read_blocks(inode_blocks + 1, dir_block_num, &directory_entry_tbl);
            // read the inode table
            read_blocks(1, inode_blocks, &inode_tbl);
            // read the bit map table
            read_blocks(1023, 1, free_bit_map);
            //initialize the file_decriptor table
			initialize_file_descriptor();
    }
}



/*
 to find next file in dirctory
 if found -> return 1
 */
int sfs_getnextfilename(char *fname){
    bool if_found=false;
  
    
    for (int i=cur_file_index; i<NUM_OF_FILES;i++){
        if (directory_entry_tbl[i].name[0] != '\0') {
            int j=0;
            
            for (; j< MAX_fname; j++){
                fname[j]=directory_entry_tbl[i].name[j];
            }
            
        fname[j]='\0';
        cur_file_index=i+1;
       
        if_found = true;
        return 1;
            
        }
    }
    if (!if_found){
        cur_file_index=0;
    }

    return 0;
}

/**
 *  return size of file
 *  return -1, if can't find the file
 *  @param path
 */
int sfs_getfilesize(const char* path){
    for (int i = 0; i < NUM_OF_FILES; i++) {
        if (strcmp(directory_entry_tbl[i].name, ((char*)path))==0) {
            return inode_tbl[directory_entry_tbl[i].num].size;
        }
    }
    return -1;
}

/**
 * return -1, if exceed max file name limit
 return slot of file decriptor table, if file is already opened
    so don't need to open the file again
 return empty slot, if file exist but not yet opened

 * @param name
 * @return
 */
int sfs_fopen(char *name){
    //check for file name
    if(MAX_fname < strlen(name)){
        printf("exceed maximun file name limit");
        return -1;
    }
    

    //check if the file is already opened
    //if not opened yet, the system refuse to read or write :(
    int i=0;
    int j=0;
    int k=0;
    for(; i<NUM_OF_FILES;i++){
        if(strcmp(name, directory_entry_tbl[i].name)==0){ //directory_entry_tbl in disk
            for(;j<NUM_OF_FILES;j++){
                if (file_descriptors[j].iNodeIndex ==directory_entry_tbl[i].num){
                    
                    printf("file is already opened!!file_descriptors[j].iNodeIndex=%d\n ",file_descriptors[j].iNodeIndex);
                    return j;
                }
            }
            
            //if the file exist and not yet open, so find a empty slot for it
            for (; k < NUM_OF_FILES; k++) {
                if (file_descriptors[k].inode == NULL) { // in mem
                file_descriptors[k].iNodeIndex = directory_entry_tbl[i].num;
                file_descriptors[k].inode = &inode_tbl[directory_entry_tbl[i].num];
                file_descriptors[k].read_ptr = inode_tbl[directory_entry_tbl[i].num].size;
                file_descriptors[k].write_ptr = inode_tbl[directory_entry_tbl[i].num].size;
                return k;
                }
            }
            
        }
    }
    //the file not yet exist, need to create one
    int fd=-1;
    
    for (i=0; i<NUM_INODES;i++){
        if(inode_tbl[i].size==-1 ){
            
            int num =get_index();
            if (num>= 1023)//=>no free block
                return 0;
            inode_tbl[i].data_block_ptr[0]=num;
			inode_tbl[i].size=0;
           
            for(j=0; j<NUM_OF_FILES;j++){
                if (directory_entry_tbl[j].num == -1){
                    directory_entry_tbl[j].num = i;
                    for (int k = 0; k < MAX_fname; k++) {
                        directory_entry_tbl[j].name[k] = name[k];
                    }
                    break;
                }
            }
            
            for (j = 0; j < NUM_OF_FILES; j++) {
                if (file_descriptors[j].inode == NULL) {
                    file_descriptors[j].inode = &inode_tbl[i];
                    file_descriptors[j].iNodeIndex = i;
                    file_descriptors[j].read_ptr = 0;
                    file_descriptors[j].write_ptr = 0;
                    fd = j;
                    break;
                }
            }
			break;
        }
    }
    update_disk();
    return fd;
}

/**
 *  find file and close successfully, return 0
 not find the file, return -1
 * @param fileID
 * @return
 */
int sfs_fclose(int fileID){

    if(file_descriptors[fileID].iNodeIndex==-1){//file dne
        return -1;//didn't find file
    }
	inode_t* inode = file_descriptors[fileID].inode;

	//inode.
    file_descriptors[fileID].read_ptr=0;
    file_descriptors[fileID].write_ptr=0;
    file_descriptors[fileID].iNodeIndex=-1;
    file_descriptors[fileID].inode=NULL;
    file_descriptors[fileID].iNodeIndex =-1;


    return 0;
}


/**
 * write seek
 * @param loc
 * @param fileID
 */
int sfs_fwseek(int fileID, int loc){
    if (fileID < 0 || loc < 0 || file_descriptors[fileID].iNodeIndex == -1) {
        printf("Error : input is invalid \n");
        return -1;
    }
    if (loc > file_descriptors[fileID].inode->size) {
        printf("Error : pointer exceed file size\n");
        return -1;
    }
    
    file_descriptors[fileID].write_ptr = loc;
    return 0;
}

/**
 * read seek
 * @param fileID
 * @param loc
 * @return
 */
int sfs_frseek(int fileID, int loc){
    if (fileID < 0 || loc < 0 || file_descriptors[fileID].iNodeIndex == -1) {
        printf("Error : input is invalid \n");
        return -1;
    }
    if (loc > file_descriptors[fileID].inode->size) {
        printf("Error : pointer exceed file size\n");
        return -1;
    }
    
    file_descriptors[fileID].read_ptr = loc;
    return 0;
}

/**
 * read file
 * @param fileID
 * @param buf
 * @param length
 */
int sfs_fread(int fileID, char *buf, int length){
    //int read_blocks(int start_address, int nblocks, void *buffer)
    //check if the file is opened, if not return -1

    if(length<=0){
          printf("sorry, lenth cannot be 0 or negative\n");
          return -1;
      }

  
	if(file_descriptors[fileID].iNodeIndex==-1){
            printf("the file has not been opened\n");
            return -1;
        }
  
    //finishing checking, start to read :)
    
    // if the required size is lager than the file size, only read the portion we have
     int file_size= file_descriptors[fileID].inode->size;

	 if ((file_descriptors[fileID].read_ptr + length) > file_size){
	     length=(file_size - file_descriptors[fileID].read_ptr);
     }else{
	     length=length;
	 }


    int read_length;

	int cur_offset = file_descriptors[fileID].read_ptr % BLOCK_SIZE;

    if (length > BLOCK_SIZE - cur_offset){
        read_length= BLOCK_SIZE - cur_offset;
    }else{
        read_length=length;
    }
    int actual_read_len=read_length;
    
    inode_t *cur_inode = &inode_tbl[file_descriptors[fileID].iNodeIndex];
	int ind_ptr_tlb[BLOCK_SIZE / 4];

	if (cur_inode->ind_ptr != -1) {
		read_blocks(cur_inode->ind_ptr, 1, ind_ptr_tlb);
	}

    //cur_block is block index
    int cur_block = file_descriptors[fileID].read_ptr / BLOCK_SIZE;

    char read_buffer[BLOCK_SIZE];
    
  // sep1: to read the first block, since it might not be full block size
    
    int block_ptr=0;
    if (cur_block<12){ //don't need to use ind. ptr
        read_blocks(cur_inode->data_block_ptr[cur_block], 1, read_buffer);

    }else{//use ind. ptr
        //read_blocks(cur_inode->ind_ptr, 1, ind_ptr_tlb);
        block_ptr=ind_ptr_tlb[cur_block-12];
        read_blocks(block_ptr, 1, read_buffer);
    }
        
    //int first_read_block_len;
   
    memcpy(buf, read_buffer + cur_offset, read_length);
	file_descriptors[fileID].read_ptr += read_length;
    int cur_read_len = read_length;
    int len_of_next_block=0;
	length-=read_length;
	cur_block++;
    //step 2, keep read until the end
    while(length >0){
		
          if (cur_block<12){ //don't need to use ind. ptr
               read_blocks(cur_inode->data_block_ptr[cur_block], 1, read_buffer);
          }else{              
              read_blocks(ind_ptr_tlb[cur_block-12], 1, read_buffer);
			  block_ptr++;
          }
        
        
        if(length > BLOCK_SIZE){
            len_of_next_block=BLOCK_SIZE;
        }else{
            len_of_next_block= length;
        }
        
        memcpy(buf + cur_read_len, read_buffer, len_of_next_block);
        cur_read_len += len_of_next_block;
        file_descriptors[fileID].read_ptr += len_of_next_block;
		length -=len_of_next_block;
        cur_block++;
		actual_read_len += len_of_next_block;
			
      }
      
    return actual_read_len;
   
}

/**
 * write file
 * @param fileID
 * @param buf
 * @param length
 */
int sfs_fwrite(int fileID, char *buf, int length){
    //int read_blocks(int start_address, int nblocks, void *buffer)
    //check if the file is opened, if not return -1
    if(length<=0){
          printf("sorry, lenth cannot be 0 or negative\n");
          return -1;
      }
    for(int i=0; i<NUM_OF_FILES;i++){
        if(directory_entry_tbl[i].num==-1){
            printf("the file has not been opened, need to open it first then you can write\n");
            return -1;
        }
    if(directory_entry_tbl[i].num==file_descriptors[fileID].iNodeIndex){
        break;
        }
    }
    
    //finishing checking, start to write :)
    int cur_len = length;
    int len_written = 0;
    int ind_ptr_tlb[BLOCK_SIZE/4];
    int block_ptr;
    int block_num;
    inode_t* cur_inode = file_descriptors[fileID].inode;
    int cur_block= file_descriptors[fileID].write_ptr/BLOCK_SIZE;
    char write_temp[BLOCK_SIZE];
    
     //step1: read 1st block from disk to wirte_temp
	if (cur_inode->ind_ptr != -1) {
		read_blocks(cur_inode->ind_ptr, 1, ind_ptr_tlb);
	}

    if(cur_block>11){//if >11, need to create a table for extra pointers
       
        if(cur_inode->ind_ptr==-1){
            int num=get_index();
            if (num >= 1022)
                 return 0;
           cur_inode->ind_ptr=num;
		   memset(ind_ptr_tlb, 0, BLOCK_SIZE);
        }
        
        //Clean the table         
      
         block_ptr=ind_ptr_tlb[cur_block-12];
        //get free blocks and link to ptrs in the block pointed by ind. ptr
         if (block_ptr==0) {
             block_num = get_index();
              if (block_num >= 1022)
                  return 0;
             block_ptr=block_num;
             ind_ptr_tlb[cur_block-12]=block_ptr;
         }
        
         read_blocks(block_ptr, 1, write_temp);
    }else{
         read_blocks(cur_inode->data_block_ptr[cur_block], 1, write_temp);
    }
    
    int write_len; //the size of the first block, it may not a whole block to write into
    int offset=file_descriptors[fileID].write_ptr % BLOCK_SIZE;

    if(cur_len <= BLOCK_SIZE - offset){
        write_len=cur_len;
    }else{
        write_len=BLOCK_SIZE - offset;
    }
    
    memcpy(write_temp + offset, buf, write_len);
    
    //step2: write 1st block from temp to file
    if(cur_block>11){
        write_blocks(block_ptr, 1, write_temp);
    }else{
        write_blocks(cur_inode->data_block_ptr[cur_block], 1, write_temp);
    }
    
    cur_block++;
    len_written += write_len;
    cur_len -= write_len;
    file_descriptors[fileID].write_ptr += write_len;
    
    //step 3,keep writ euntil finished
    while (cur_len > 0) {
        /*
         if >11, need to use ind_ptr
         1. need to find a new block to laod all the ptrs
         2. link the ptrs to a new empty data block
         else
         1. link the ptrs to a new empty data block
         */
		if (cur_block > 266) {
			return -1;
		}

        if(cur_block>11){
            //if this is the first time using ind_ptr, find block to store all the ptrs
			
            if(cur_inode->ind_ptr==-1){
                      int num=get_index();
                      if (num >= 1022)
                           return 0;
                     cur_inode->ind_ptr=num;
					 memset(ind_ptr_tlb, 0, BLOCK_SIZE);
                  }
            
            
            
            block_ptr=ind_ptr_tlb[cur_block-12];
            
            //get free blocks and link to ptrs in the block pointed by ind. ptr
            if (block_ptr==0) {
                block_num = get_index();
                if (block_num >= 1022)
                      return 0;
                block_ptr=block_num;
                ind_ptr_tlb[cur_block-12]=block_ptr;
             }
            
        }else{
            //if the inode ptr has not pointed to any data block, assign one for it
            if (cur_inode->data_block_ptr[cur_block]==-1) {
               block_num = get_index();
               if (block_num >= 1022)
                     return 0;
               block_ptr=block_num;
			   cur_inode->data_block_ptr[cur_block] = block_ptr;
               
            }
        }
        
        int write_len=0;
        if (cur_len>BLOCK_SIZE){
            write_len=BLOCK_SIZE;
        }else{
            write_len=cur_len;
        }
        
        if(cur_block>11){
             memcpy(write_temp, buf + len_written, write_len);
             write_blocks(block_ptr, 1, write_temp);
        }else{
             memcpy(write_temp, buf + len_written, write_len);
             write_blocks(cur_inode->data_block_ptr[cur_block], 1, write_temp);
        }

        len_written += write_len;
        cur_len -= write_len;
        cur_block++;
        file_descriptors[fileID].write_ptr += write_len;
        
    }
    
    if(file_descriptors[fileID].write_ptr > file_descriptors[fileID].inode->size){
        
        file_descriptors[fileID].inode->size =file_descriptors[fileID].write_ptr;
        
    }else{
        file_descriptors[fileID].inode->size =file_descriptors[fileID].inode->size ;
    }
    
	if (cur_inode->ind_ptr != -1) {
		write_blocks(cur_inode->ind_ptr, 1, ind_ptr_tlb);
	}
    update_disk();
    return len_written;

}

int sfs_remove(char *file){
    //find the file in the  directory_entry_tbl

    for (int i = 0; i < NUM_OF_FILES; i++) {
        if(strcmp(directory_entry_tbl[i].name, file)==0){
            
        //check if the file is already opened, if yes, file will fail (return code != 0)
            if(file_descriptors[directory_entry_tbl[i].num].iNodeIndex !=-1){
                printf("the file has been opened, can not remove it\n");
                return -1;
            }
                       
            // remove in inode table
            inode_t* cur_node = &inode_tbl[directory_entry_tbl[i].num];
            //1. remove inode ptr
            for (int j = 0; j < 12; j++) {
                if (cur_node->data_block_ptr[j] != -1) {
                    rm_index(cur_node->data_block_ptr[j]);
                    cur_node->data_block_ptr[j] = -1;
                }
            }
            
            //2. remove ind ptr
             //memset( ind_ptr_tlb, 0, BLOCK_SIZE);
            if(cur_node->ind_ptr!=-1){
				rm_index(cur_node->ind_ptr);
				cur_node->ind_ptr=-1;
				
            }
            
            //3. remove everything else
            cur_node->size = -1;
            cur_node->mode = 0;
            cur_node->link_cnt = 0;
            cur_node->uid = 0;
            cur_node->gid = 0;
            
            // remove file descriptors
            for (int j = 0; j < MAX_fname; j++) {
                if (directory_entry_tbl[i].num == file_descriptors[j].iNodeIndex) {
                    file_descriptors[j].iNodeIndex = -1;
                    file_descriptors[j].read_ptr = 0;
                    file_descriptors[j].write_ptr = 0;
                    file_descriptors[j].inode = NULL;
                }
            }
            
			//remove file name
			for (int j = 0; j < MAX_fname; j++) {
				directory_entry_tbl[i].name[j] = '\0';
			}

            //remove file num in directory table
            directory_entry_tbl[i].num = -1;
        }
    }
    update_disk();


    return 0;
}


/*
 return correct index of unsed block
 or, return -1, if all blocks are used
 */


int get_index()
{
	int i = 0;
	for (int i = 0; i < NUM_BLOCKS; i++) {
		if (free_bit_map[i] != 0) {
			free_bit_map[i] = 0;
			return i;
		}
	}

	return -1;
}

void rm_index(uint32_t index)
{
	free_bit_map[index] = 1;
}
