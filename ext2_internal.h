#ifndef EXT2_INTERNAL_H
#define EXT2_INTERNAL_H

#include "ext2.h"

int read_superblock(ext2_t* config, ext2_superblock_t* superblk);
int read_block_group_descriptor(ext2_t* ext2, uint32_t group, ext2_block_group_descriptor_t* bgd);

#endif
