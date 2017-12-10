int init_fresh_disk(char *filename, int block_size, int num_blocks);
int init_disk(char *filename, int block_size, int num_blocks);
// start_address = index of block, not address of block, very bad argument name :(
int read_blocks(int start_address, int nblocks, void *buffer);
int write_blocks(int start_address, int nblocks, void *buffer);
int close_disk();
