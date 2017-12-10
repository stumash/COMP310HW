#include "sfs_api.h"
#include "bitmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fuse.h>
#include <strings.h>
#include "disk_emu.h"



#define STUART_MASHAAL_DISK "sfs_disk.disk"

// set _which_bit to 1 in _data
#define FREE_BIT(_data, _which_bit) \
    _data = _data | (1 << _which_bit)
// set _which_bit to 0 in _data
#define USE_BIT(_data, _which_bit) \
    _data = _data & ~(1 << _which_bit)



/**--------------------------------------------------------------------------------------
 * NUMERICAL CONSTANTS
 *-------------------------------------------------------------------------------------*/

#define NUM_BLOCKS      1024 // maximum number of data blocks on the disk.
#define BYTES_PER_BLOCK 1024 // num bytes per block
/**
 * A file is at least one block, and there data 1024 blocks.
 * So, max 1024 files which means max 1024 inodes
 * For convenience, an inode will be 1024 / 16 = 64 bytes. (16 inodes per block). Therefore,
 *
 * 1 superblock
 * 32 blocks of inodes
 * 1024 data blocks
 * 1 bitmap block (data block bitmap AND inode bitmap)
 * total = 1024 + 32 + 2 = 1058 blocks of 1024 bytes each
 *
 * first: 1 superblock
 * second: 32 inode blocks
 * third: 1024 data blocks
 * fourth: 1 bitmap block
 */
#define FILESYSTEM_TOTAL_NUM_BLOCKS 1058

#define SUPERBLOCK_SIZE             160  // num bytes in superblock, excluding trailing 0 s
#define INODE_SIZE                  64   // num bytes per inode
#define NUM_INODE_BLOCKS            64   // num blocks of inodes

#define SUPERBLOCK_BLOCK_INDEX      0
#define FIRST_INODE_BLOCK_INDEX     1
#define FIRST_DATA_BLOCK_INDEX      33
#define BITMAP_BLOCK_INDEX          1057

#define SUPERBLOCK_START_ADDRESS      (SUPERBLOCK_BLOCK_INDEX * BYTES_PER_BLOCK)
#define FIRST_INODE_START_ADDRESS     (FIRST_DATA_BLOCK_INDEX * BYTES_PER_BLOCK)
#define FIRST_DATA_BLOCK_START_ADRESS (FIRST_DATA_BLOCK_INDEX * BYTES_PER_BLOCK)
#define BITMAP_START_ADDRESS          (BITMAP_BLOCK_INDEX     * BYTES_PER_BLOCK)

#define MAX_FILENAME_SIZE           28 // at most 27 characters per filename, excluding \0
#define MAX_FILENAME_B4_DOT         23
#define MAX_FILENAME_EXT            3

#define BITMAP_ROW_SIZE (NUM_BLOCKS/8) // number of bytes needed to have 1 bit for each block

// get block address from inode number.  recall two inodes per block!
#define INODE_NUMBER_TO_BLOCK_ADDRESS(_inode_number) \
    (FIRST_INODE_START_ADDRESS + ((_inode_number / 16) * BYTES_PER_BLOCK))
// get address of inode from inode number
#define INODE_NUMBER_TO_ADDRESS(_inode_number) \
    (FIRST_INODE_START_ADDRESS + (_inode_number * INODE_SIZE))
// get index of block containing inode from inode number
#define INODE_NUMBER_TO_BLOCK_INDEX(_inode_number) \
    (FIRST_INODE_BLOCK_INDEX + (_inode_number / 16))



/*-----------------------------------------------------------------------------------------
 *  NECESSARY DATA STRUCTURES: BITMAP, INODE TABLE, SUPERBLOCK, DIR_ENTRY, OPEN_FILE_TABLE
 *---------------------------------------------------------------------------------------*/

/*--------------------------------------------------
 *  THE BITMAP
 *------------------------------------------------- */
// initialize all blocks free except first data block for root dir
static uint8_t free_bitmap[BITMAP_ROW_SIZE] = { 0xFE, [1 ... BITMAP_ROW_SIZE - 1] = UINT8_MAX };

/*--------------------------------------------------
 *  INODE
 *------------------------------------------------- */
typedef struct _st_inode
{
    uint32_t mode;
    uint32_t link_cnt;
    uint8_t  uid;
    uint8_t  gid;
    uint16_t size; // number of bytes
    uint32_t blk_pntrs[12]; // actual block numbers on disk
    uint32_t ind_pntr; // inode idx in inode_table
} st_inode;

static st_inode default_root_dir_inode = {
    .mode      = 0x000001FD, // initalize to rwxrwxr-x
    .link_cnt  = 1,
    .uid       = 1000,
    .gid       = 1000,
    .size      = 0,
    .blk_pntrs = {FIRST_DATA_BLOCK_INDEX, 0,0,0,0,0,0,0,0,0,0,0},
    .ind_pntr  = 0 // an inode number
};

// default bitmap for inodes in use.  first inode is used for root dir
static uint8_t inode_bitmap[BITMAP_ROW_SIZE] = { 0xFE, [1 ... BITMAP_ROW_SIZE - 1] = UINT8_MAX };

// in-memory cache of inode table. default first elem is default_root_dir_inode
static st_inode inode_table[NUM_BLOCKS] = {0};

/*--------------------------------------------------
 *  SUPERBLOCK
 *------------------------------------------------- */
typedef struct _st_superblock
{
    uint32_t magic;             // type of filesystem, a magic number
    uint32_t block_size;        // bytes per block, 1024
    uint32_t file_system_size;  // number of blocks in filesystem, 1538
    uint32_t inode_table_length;// number of blocks in inode table, 512
    uint32_t root_dir_inode_num;// inode number of block containing root directory, 0
} st_superblock;

static st_superblock default_superblock = {
    .magic              = 0xACBD0005,
    .block_size         = BYTES_PER_BLOCK,
    .file_system_size   = FILESYSTEM_TOTAL_NUM_BLOCKS,
    .inode_table_length = INODE_SIZE,
    .root_dir_inode_num = 0
};

/*--------------------------------------------------
 *  DIRECTORY_ENTRY
 *------------------------------------------------- */
typedef struct _dir_entry
{
    uint32_t inode_number;
    uint8_t filename[MAX_FILENAME_SIZE];
} dir_entry; // 32 bytes total

static dir_entry dir_cache[NUM_BLOCKS] = {0};

/*--------------------------------------------------
 *  OPEN FILE DESCRIPTORS
 *------------------------------------------------- */
typedef struct _fd_table_entry
{
    uint32_t inode_number;
    uint32_t rw_pointer; // an offset
} fd_table_entry; // 8 bytes total

static uint32_t num_open_fd = 0; // num of open file descriptors
static fd_table_entry open_fd_table[NUM_BLOCKS] =  {0};



/*--------------------------------------------------
 *  MY HELPERS
 *------------------------------------------------- */
void write_bitmaps_and_inode_table() {
    // write bitmap to disk
    uint8_t bitmap_buffer[BYTES_PER_BLOCK] = {0};
    memcpy(bitmap_buffer, free_bitmap, sizeof(free_bitmap));
    memcpy(bitmap_buffer + sizeof(free_bitmap), inode_bitmap, sizeof(inode_bitmap));
    write_blocks(BITMAP_BLOCK_INDEX, 1, &bitmap_buffer);

    // write inode table to disk
    write_blocks(FIRST_INODE_BLOCK_INDEX, NUM_INODE_BLOCKS, inode_table);
}



/*--------------------------------------------------
 *  THE SFS API
 *------------------------------------------------- */

void mksfs(int fresh) {
    if (fresh)
    {
        init_fresh_disk(STUART_MASHAAL_DISK, BYTES_PER_BLOCK, FILESYSTEM_TOTAL_NUM_BLOCKS);

        // write default superblock to disk
        uint8_t superblock_buffer[BYTES_PER_BLOCK] = {0};
        memcpy(superblock_buffer, &default_superblock, sizeof(default_superblock));
        write_blocks(SUPERBLOCK_BLOCK_INDEX, 1, &superblock_buffer);

        // write default bitmap to disk
        uint8_t bitmap_buffer[BYTES_PER_BLOCK] = {0};
        memcpy(bitmap_buffer, free_bitmap, sizeof(free_bitmap));
        memcpy(bitmap_buffer + sizeof(free_bitmap), inode_bitmap, sizeof(inode_bitmap));
        write_blocks(BITMAP_BLOCK_INDEX, 1, &bitmap_buffer);

        // write default inode table to disk
        inode_table[0] = default_root_dir_inode;
        write_blocks(FIRST_INODE_BLOCK_INDEX, NUM_INODE_BLOCKS, inode_table);
    }
    else
    {
        init_disk(STUART_MASHAAL_DISK, BYTES_PER_BLOCK, FILESYSTEM_TOTAL_NUM_BLOCKS);

        // read in bitmaps
        uint8_t bitmap_buffer[BYTES_PER_BLOCK] = {0};
        read_blocks(BITMAP_BLOCK_INDEX, 1, bitmap_buffer);
        memcpy(free_bitmap, bitmap_buffer, sizeof(free_bitmap));
        memcpy(inode_bitmap, bitmap_buffer + sizeof(free_bitmap), sizeof(inode_bitmap));

        // read in inode table
        read_blocks(FIRST_INODE_BLOCK_INDEX, NUM_INODE_BLOCKS, inode_table);

        // read in directory to dir_cache
        st_inode root_dir_inode = inode_table[0];
        uint32_t root_dir_num_blocks  = root_dir_inode.size / BYTES_PER_BLOCK;
        if (root_dir_inode.size % BYTES_PER_BLOCK) root_dir_num_blocks += 1;
        uint32_t *root_dir_contents_bytes = (uint8_t*)dir_cache;
        for (int i = 0; i < root_dir_num_blocks; ++i)
        {
            if (i < 12) // direct block pointers
            {
                uint32_t block_address = root_dir_inode.blk_pntrs[i];
                read_blocks(block_address, 1, root_dir_contents_bytes + i * BYTES_PER_BLOCK);
            }
            else // indirect block pointer
            {
                uint32_t block_address = inode_table[root_dir_inode.ind_pntr].blk_pntrs[i % 12];
                read_blocks(block_address, 1, root_dir_contents_bytes + i * BYTES_PER_BLOCK);
            }
        }
    }
}
int sfs_getnextfilename(char *fname){

}
int sfs_getfilesize(const char* path){

}
int sfs_fopen(char *name){
    // does the file exist. if so what is inode number
    uint32_t file_inode = 0;
    uint32_t file_first_block = 0;
    for (int dir_entry_index = 0; dir_entry_index < NUM_BLOCKS; ++dir_entry_index) {
        if (strcmp(dir_cache[dir_entry_index].filename, name) == 0) {
            file_inode = dir_cache[dir_entry_index].inode_number;
        }
    }

    if (file_inode) {// if file exists, check if already open
        for (int fd_table_index = 0; fd_table_index < NUM_BLOCKS; ++fd_table_index) {
            if (open_fd_table[fd_table_index].inode_number == file_inode) {
                return fd_table_index;
            }
        }
    } else { // file doesn't exist, create inode and directory entry

        int found_first_free_inode = 0;
        for (int inode_bitmap_idx = 0; inode_bitmap_idx < BITMAP_ROW_SIZE; ++inode_bitmap_idx) {
            if (found_first_free_inode) break;
            if (inode_bitmap[inode_bitmap_idx] > 0) {
                for (int bit_idx = 0; bit_idx < 8; ++bit_idx) { //
                    if (found_first_free_inode) break;
                    if (inode_bitmap[inode_bitmap_idx] & (1 << bit_idx)) {
                        USE_BIT(inode_bitmap[inode_bitmap_idx], bit_idx);
                        file_inode = inode_bitmap_idx * 8 + bit_idx;
                        found_first_free_inode = 1;
                    }
                }
            }
        }
        if (!found_first_free_inode) { printf("NO INODE BLOCKS FREE\n"); return -1; }

        int found_first_free_data_block = 0;
        for (int free_bitmap_idx = 0; free_bitmap_idx < BITMAP_ROW_SIZE; ++free_bitmap_idx) {
            if (found_first_free_data_block) break;
            if (free_bitmap[free_bitmap_idx] > 0) {
                for (int bit_idx = 0; bit_idx < 8; ++bit_idx) {
                    if (found_first_free_data_block) break;
                    if (free_bitmap[free_bitmap_idx] & (1 << bit_idx)) {
                        USE_BIT(free_bitmap[free_bitmap_idx], bit_idx);
                        file_first_block = FIRST_DATA_BLOCK_INDEX + free_bitmap_idx*8 + bit_idx;
                        found_first_free_data_block = 1;
                    }
                }
            }
        }
        if (!found_first_free_data_block) { printf("NO DATA BLOCKS FREE\n"); return -1; }

        // create inode
        inode_table[file_inode] = default_root_dir_inode;
        inode_table[file_inode].blk_pntrs[0] = file_first_block;

        // create directory entry, set file_inode and filename
        for (int dir_entry_idx = 0; dir_entry_idx < NUM_BLOCKS; ++dir_entry_idx) {
            if (dir_cache[dir_entry_idx].inode_number == 0) {
                dir_cache[dir_entry_idx].inode_number = file_inode;
                memcpy(dir_cache[dir_entry_idx].filename, name, strlen(name));
                break;
            }
        }
    }

    // write the new inode and dir_entry to disk
    write_blocks(FIRST_INODE_BLOCK_INDEX, NUM_INODE_BLOCKS, inode_table);
    // TODO: write dir entry to disk

    // put the new open file in the fd_table and return its index therein
    for (int fd_table_index = 0; fd_table_index < NUM_BLOCKS; ++fd_table_index) {
        if (open_fd_table[fd_table_index].inode_number == 0)
        {
            open_fd_table[fd_table_index].inode_number = file_inode;
            open_fd_table[fd_table_index].rw_pointer = 0;
            return fd_table_index;
        }
    }
}
int sfs_fclose(int fileID) {
    if (open_fd_table[fileID].inode_number == 0) {
        printf("MY_ERROR: file descriptor not found.\n");
        return -1;
    } else {
        fd_table_entry empty_fd_entry = {0};
        memcpy(open_fd_table + fileID, &empty_fd_entry, sizeof(fd_table_entry));
    }
}
int sfs_fread(int fileID, char *buf, int length) {
    //get inode of file
    int file_inode_number = 0;
    if (open_fd_table[fileID].inode_number != 0)
        file_inode_number = open_fd_table[fileID].inode_number;
    if (!file_inode_number) { printf("MY_ERROR: file descriptor not found.\n"); return -1; }
    st_inode file_inode = inode_table[file_inode_number];
    // load entire file into a buffer
    uint8_t file_buffer[file_inode.size];
    memset(file_buffer, 0x00, sizeof(file_buffer));
    int file_num_blocks = file_inode.size / BYTES_PER_BLOCK;
    if (file_inode.size % BYTES_PER_BLOCK != 0) file_num_blocks += 1;
    for (int block_in_file_idx = 0; block_in_file_idx < file_num_blocks; ++block_in_file_idx) {
        if (block_in_file_idx < 12) { // first 12 direct block pointer
            uint32_t block_address = file_inode.blk_pntrs[block_in_file_idx];
            read_blocks(block_address, 1, file_buffer + block_in_file_idx * BYTES_PER_BLOCK);
        } else { // indirect block pointer
            uint32_t block_address = inode_table[file_inode.ind_pntr].blk_pntrs[block_in_file_idx % 12];
            read_blocks(block_address, 1, file_buffer + block_in_file_idx * BYTES_PER_BLOCK);
        }
    }
    // while loop to copy buffer into *buf, starting at rw_pointer offset
    int i;
    for (i = open_fd_table[fileID].rw_pointer; i < length && i < file_inode.size; ++i) {
        buf[i] = file_buffer[i];
    }
    return i - open_fd_table[fileID].rw_pointer;
}
int sfs_fwrite(int fileID, const char *buf, int length) {
    // get new size of file with additional text, make buffer of that size
    int file_inode_number = 0;
    if (open_fd_table[fileID].inode_number != 0)
        file_inode_number = open_fd_table[fileID].inode_number;
    if (!file_inode_number) { printf("MY_ERROR: file descriptor not found.\n"); return -1; }
    st_inode file_inode = inode_table[file_inode_number];
    int file_size_new = file_inode.size;
    if (open_fd_table[fileID].rw_pointer + length > file_inode.size)
        file_size_new = open_fd_table[fileID].rw_pointer + length;
    uint8_t file_buffer[file_size_new];
    memset(file_buffer, 0x00, sizeof(file_buffer));
    // read entire file into the buffer
    int file_num_blocks = file_inode.size / BYTES_PER_BLOCK;
    if (file_inode.size % BYTES_PER_BLOCK != 0) file_num_blocks += 1;
    for (int block_in_file_idx = 0; block_in_file_idx < file_num_blocks; ++block_in_file_idx) {
        if (block_in_file_idx < 12) { // first 12 direct block pointer
            uint32_t block_address = file_inode.blk_pntrs[block_in_file_idx];
            read_blocks(block_address, 1, file_buffer + block_in_file_idx * BYTES_PER_BLOCK);
        } else { // indirect block pointer
            uint32_t block_address = inode_table[file_inode.ind_pntr].blk_pntrs[block_in_file_idx % 12];
            read_blocks(block_address, 1, file_buffer + block_in_file_idx * BYTES_PER_BLOCK);
        }
    }
    // write new text to the buffer at rw_pointer
    for (int i = open_fd_table[fileID].rw_pointer; i < file_size_new; ++i) {
        file_buffer[i] = buf[i];
    }
    // update inode file size
    file_inode.size = file_size_new;
    // write the file to disk, creating inode block pointers as necesary
    file_num_blocks = file_inode.size / BYTES_PER_BLOCK;
    if (file_inode.size % BYTES_PER_BLOCK != 0) file_num_blocks += 1;
    for (int file_block_idx = 0; file_block_idx < file_num_blocks; ++file_block_idx) {
        if (file_block_idx < 12) { // first 12 direct block pointer
            uint32_t block_address = file_inode.blk_pntrs[file_block_idx];
            if (block_address != 0) {
                write_blocks(block_address, 1, file_buffer + file_block_idx * BYTES_PER_BLOCK);
            } else {
                int found_first_free_data_block = 0;
                for (int free_bitmap_idx = 0; free_bitmap_idx < BITMAP_ROW_SIZE; ++free_bitmap_idx) {
                    if (found_first_free_data_block) break;
                    if (free_bitmap[free_bitmap_idx] > 0) {
                        for (int bit_idx = 0; bit_idx < 8; ++bit_idx) {
                            if (found_first_free_data_block) break;
                            if (free_bitmap[free_bitmap_idx] & (1 << bit_idx)) {
                                USE_BIT(free_bitmap[free_bitmap_idx], bit_idx);
                                found_first_free_data_block = FIRST_DATA_BLOCK_INDEX + free_bitmap_idx*8 + bit_idx;
                            }
                        }
                    }
                }
                if (!found_first_free_data_block) { printf("NO DATA BLOCKS FREE\n"); return -1; }
                file_inode.blk_pntrs[file_block_idx] = found_first_free_data_block;
                write_blocks(found_first_free_data_block, 1, file_buffer + file_block_idx * BYTES_PER_BLOCK);
            }
        } else { // indirect block pointer
            if (!file_inode.ind_pntr) {
                // set up inode for indirect pointer
                int found_first_free_inode = 0;
                for (int inode_bitmap_idx = 0; inode_bitmap_idx < BITMAP_ROW_SIZE; ++inode_bitmap_idx) {
                    if (found_first_free_inode) break;
                    if (inode_bitmap[inode_bitmap_idx] > 0) {
                        for (int bit_idx = 0; bit_idx < 8; ++bit_idx) { //
                            if (found_first_free_inode) break;
                            if (inode_bitmap[inode_bitmap_idx] & (1 << bit_idx)) {
                                USE_BIT(inode_bitmap[inode_bitmap_idx], bit_idx);
                                found_first_free_inode = inode_bitmap_idx * 8 + bit_idx;
                            }
                        }
                    }
                }
                if (!found_first_free_inode) { printf("NO INODE BLOCKS FREE\n"); return -1; }
                file_inode.ind_pntr = found_first_free_inode;
                inode_table[found_first_free_inode] = default_root_dir_inode;
                inode_table[found_first_free_inode].blk_pntrs[0] = 0;
            }
            uint32_t block_address = inode_table[file_inode.ind_pntr].blk_pntrs[file_block_idx % 12];
            if (block_address != 0) {
                write_blocks(block_address, 1, file_buffer + file_block_idx * BYTES_PER_BLOCK);
            } else {
                int found_first_free_data_block = 0;
                for (int free_bitmap_idx = 0; free_bitmap_idx < BITMAP_ROW_SIZE; ++free_bitmap_idx) {
                    if (found_first_free_data_block) break;
                    if (free_bitmap[free_bitmap_idx] > 0) {
                        for (int bit_idx = 0; bit_idx < 8; ++bit_idx) {
                            if (found_first_free_data_block) break;
                            if (free_bitmap[free_bitmap_idx] & (1 << bit_idx)) {
                                USE_BIT(free_bitmap[free_bitmap_idx], bit_idx);
                                found_first_free_data_block = FIRST_DATA_BLOCK_INDEX + free_bitmap_idx*8 + bit_idx;
                            }
                        }
                    }
                }
                if (!found_first_free_data_block) { printf("NO DATA BLOCKS FREE\n"); return -1; }
                st_inode ind_inode = inode_table[file_inode.ind_pntr];
                ind_inode.blk_pntrs[file_block_idx % 12] = found_first_free_data_block;
                write_blocks(found_first_free_data_block, 1, file_buffer + file_block_idx * BYTES_PER_BLOCK);
            }
        }
    }
    // write inode table and bitmaps to disk
    write_bitmaps_and_inode_table();
    return length;
}
int sfs_fseek(int fileID, int loc) {
    if (open_fd_table[fileID].inode_number == 0) {
        printf("MY_ERROR: file descriptor not found.\n");
        return -1;
    } else {
        st_inode inode = inode_table[open_fd_table[fileID].inode_number];
        if (loc < 0 || loc > inode.size) {
            printf("MY_ERROR: loc argument out of range.\n");
            return -1;
        } else {
            open_fd_table[fileID].rw_pointer = loc;
        }
    }
}
int sfs_remove(char *file) {

}
