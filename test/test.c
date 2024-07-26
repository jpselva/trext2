#include "ext2.h"
#include <stdio.h>
#include <stdint.h>
#define DISKIMG_FILE "disk.img"
#define BLOCK_SZ_BYTES 1024

FILE* diskimg_fd;

int readblock(uint32_t blocknum, void* buffer) {
    long byte = blocknum * BLOCK_SZ_BYTES; 

    // set read/write position to start of block
    fseek(diskimg_fd, SEEK_SET, byte);

    // read 1 string of size BLOCK_SZ_BYTES into buffer
    int items_read = fread(buffer, BLOCK_SZ_BYTES, 1, diskimg_fd) == 0;

    return (items_read < 1) ? -1 : 0;
}

int main() {
    diskimg_fd = fopen(DISKIMG_FILE, "r");
    return 0;
}
