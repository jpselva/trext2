#include "ext2.h"

#ifdef EXT2_TESTING
#include "ext2_internal.h"
#endif

int ext2_mount(ext2_t* ext2, ext2_config_t* cfg) {
    ext2->config = *cfg; 
    return 0; 
}

int read_superblock(ext2_t* ext2, ext2_superblock_t* superblk) {
    return ext2->config.read(1024, sizeof(ext2_superblock_t), superblk);
}
