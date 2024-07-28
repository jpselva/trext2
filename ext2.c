#include "ext2.h"

#ifdef EXT2_TESTING
#include "ext2_internal.h"
#endif

int read_superblock(ext2_t* ext2, ext2_superblock_t* superblk) {
    // superblock starts at 1024th byte
    return ext2->config.read(1024, sizeof(ext2_superblock_t), superblk);
}

int read_block_group_descriptor(ext2_t* ext2, uint32_t group, ext2_block_group_descriptor_t* bgd) {
    // first block of the block group descriptor table
    uint32_t first_block = (ext2->block_size > 1024) ? 1 : 2; 

    // address of the block block group descriptor
    uint32_t start = first_block * ext2->block_size + group * sizeof(ext2_block_group_descriptor_t);
    return ext2->config.read(start, sizeof(ext2_block_group_descriptor_t), bgd);
}

int ext2_mount(ext2_t* ext2, ext2_config_t* cfg) {
    ext2_superblock_t superblk;

    ext2->config = *cfg; 
    read_superblock(ext2, &superblk);
    
    ext2->block_size = 1024 << superblk.log_block_size;

    // if the block size is huge (> 2 GiB), block_size will overflow
    if (ext2->block_size == 0) {
        return -1; 
    }

    return 0; 
}

