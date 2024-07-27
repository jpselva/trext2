#ifndef EXT2_INTERNAL_H
#define EXT2_INTERNAL_H

#include "ext2.h"

int read_superblock(ext2_t* config, ext2_superblock_t* superblk);

#endif
