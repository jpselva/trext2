#include "ext2.h"

ext2_error_t read_superblock(ext2_t* ext2, ext2_superblock_t* superblk) {
    // superblock starts at 1024th byte
    return ext2->read(1024, sizeof(ext2_superblock_t), 
                      superblk, ext2->context);
}

ext2_error_t read_block_group_descriptor(ext2_t* ext2, uint32_t group, 
        ext2_block_group_descriptor_t* bgd) {
    // size of a block group descriptor
    uint32_t size = sizeof(ext2_block_group_descriptor_t);

    // first block of the block group descriptor table
    uint32_t first_block = (ext2->block_size > 1024) ? 1 : 2; 

    // address of the block group descriptor
    uint32_t start = first_block * ext2->block_size + group * size;

    return ext2->read(start, size, bgd, ext2->context);
}

ext2_error_t ext2_mount(ext2_t* ext2, ext2_config_t* cfg) {
    ext2_superblock_t superblk;

    ext2->read = cfg->read;
    ext2->context = cfg->context;

    int superblk_error = read_superblock(ext2, &superblk);

    if (superblk_error < 0)
        return superblk_error;
    
    ext2->block_size = 1024 << superblk.log_block_size;

    // if the block size is huge (> 2 GiB), block_size will overflow
    if (ext2->block_size == 0) {
        return EXT2_ERR_BIG_BLOCK;
    }

    return 0; 
}

