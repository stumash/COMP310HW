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
// return nonzero (true) if _which_bit is set to 1 in _data
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
 *  (AND ASSOCIATED HELPER FUNCTIONS)
 *---------------------------------------------------------------------------------------*/

/*--------------------------------------------------
 *  THE BITMAP
 *------------------------------------------------- */
// initialize all blocks free except first data block for root dir
static uint8_t free_bitmap[BITMAP_ROW_SIZE] = { 0x7F, [1 ... BITMAP_ROW_SIZE - 1] = UINT8_MAX };

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
    uint32_t blk_pntrs[12];
    uint32_t ind_pntr;
} st_inode;

static st_inode default_root_dir_inode = {
    .mode      = 0x000001FD, // initalize to rwxrwxr-x
    .link_cnt  = 1,
    .uid       = 1000,
    .gid       = 1000,
    .size      = 0,
    .blk_pntrs = {FIRST_DATA_BLOCK_START_ADRESS, 0,0,0,0,0,0,0,0,0,0,0},
    .ind_pntr  = 0 // an inode number
};

// default bitmap for inodes in use.  first inode is used for root dir
static uint8_t inode_bitmap[BITMAP_ROW_SIZE] = { 0x7F, [1 ... BITMAP_ROW_SIZE - 1] = UINT8_MAX };

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

static dir_entry dir_cache[NUM_BLOCKS / 32] = {0};

/*--------------------------------------------------
 *  OPEN FILE DESCRIPTORS
 *------------------------------------------------- */
typedef struct _fd_table_entry
{
    uint32_t inode_number;
    uint32_t rw_pointer;
} fd_table_entry; // 8 bytes total

static uint32_t num_open_fd = 0; // num of open file descriptors
static open_fd_table[NUM_BLOCKS] =  {0};

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

        // read in directory
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

}
int sfs_fclose(int fileID) {

}
int sfs_fread(int fileID, char *buf, int length) {

}
int sfs_fwrite(int fileID, const char *buf, int length) {

}
int sfs_fseek(int fileID, int loc) {

}
int sfs_remove(char *file) {

}
