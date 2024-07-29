#include "utils.h"

int readblock(uint32_t start, uint32_t size, void* buffer, void* context) {
    FILE* diskfile = fopen((char*) context, "rw");

    if (diskfile == NULL)
        return -1;

    // set read/write position to start of block
    fseek(diskfile, start, SEEK_SET);

    // read 1 string of 'size' elements into buffer
    size_t items_read = fread(buffer, size, 1, diskfile);

    fclose(diskfile);

    return (items_read < 1) ? -1 : 0;
}
