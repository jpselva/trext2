#include "ext2.h"

#ifdef EXT2_TESTING
#include "ext2_internal.h"
#endif

int read_superblock(ext2_t* ext2, ext2_superblock_t* superblk) {
    // superblock starts at 1024th byte
    return ext2->config.read(1024, sizeof(ext2_superblock_t), superblk);
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

