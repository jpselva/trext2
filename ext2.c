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

ext2_error_t read_inode(ext2_t* ext2, uint32_t inode_number, ext2_inode_t* inode) {
    if (inode_number > ext2->superblk.inodes_count) {
        return EXT2_ERR_INODE_NOT_FOUND;
    }

    uint32_t inodes_per_group = ext2->superblk.inodes_per_group;
    uint32_t block_group = (inode_number - 1) / inodes_per_group; 
    uint32_t offset_inside_group = (inode_number - 1) % inodes_per_group;

    ext2_block_group_descriptor_t bgd;
    ext2_error_t bgd_error = read_block_group_descriptor(ext2, block_group, &bgd);

    if (bgd_error)
        return bgd_error;

    uint32_t inode_table_addr = bgd.inode_table * ext2->block_size;
    uint32_t inode_size = sizeof(ext2_inode_t);
    uint32_t inode_addr = inode_table_addr + offset_inside_group * inode_size;

    return ext2->read(inode_addr, inode_size, inode, ext2->context);
}

uint32_t block_map(ext2_t* ext2, const ext2_inode_t* inode, uint32_t offset) {
    uint32_t blk_index = offset / ext2->block_size;

    if (blk_index < 12) {
        return inode->block[blk_index]; 
    }

    blk_index -= 11;

    uint32_t bpb = ext2->block_size / 4;
    uint32_t i1 = blk_index % bpb;
    uint32_t i3 = blk_index/(bpb * bpb);
    uint32_t i2 = blk_index/bpb - i3 * bpb;

    uint32_t indexes[3] = {i1 - 1, i2 - 1, i3 - 1};     

    int indirection = (i3 > 0) ? 3 : (i2 > 0) ? 2 : 1;

    uint32_t block = inode->block[11 + indirection];

    while (indirection > 0 && block != 0) {
        ext2->read(block * ext2->block_size + indexes[indirection - 1] * sizeof(uint32_t), 
                sizeof(uint32_t), 
                &block, 
                ext2->context);
        indirection -= 1;
    }

    return block;
}

ext2_error_t read_data(ext2_t* ext2, const ext2_inode_t* inode, uint32_t offset, 
        uint32_t size, void* buffer) {
    if (offset + size >= inode->size) {
        // access is outside of file
        return EXT2_ERR_DATA_OUT_OF_BOUNDS;
    }

    while (size > 0) {
        uint32_t datablock = block_map(ext2, inode, offset);

        uint32_t block_offset = offset % ext2->block_size;
        uint32_t remaining = ext2->block_size - block_offset;
        uint32_t bytes_to_read = (remaining < size) ? remaining : size;

        ext2_error_t error;

        if (datablock == 0) {
            memset(buffer, 0, bytes_to_read);
        } else {
            error = ext2->read(datablock * ext2->block_size + block_offset,  
                bytes_to_read, (uint8_t*)buffer + offset, ext2->context);
        }

        if (error != 0)
            return error;

        offset += bytes_to_read;
        size -= bytes_to_read;
    }

    return 0;
}

// extracts the first name before the '/' in a file path. filename should have
// at least EXT2_MAX_FILE_NAME + 1 bytes. Returns the number of chars read or a
// negative number if the file name exceeds EXT2_MAX_FILE_NAME
int parse_filename(const char* path, char* filename) {
    int i = 0;
    char c = *(path++);
    while (c != '\0' && c != '/') {
        if (i >= EXT2_MAX_FILE_NAME) {
            return -1;
        }

        filename[i++] = c;
        c = *(path++);
    }

    filename[i] = '\0';

    return i;
}

ext2_error_t get_directory_entry(ext2_t* ext2, const ext2_inode_t* parent_inode, 
        const char* name, ext2_inode_t* inode) {
    ext2_directory_entry_t entry;
    uint32_t offset = 0;
    uint32_t size = sizeof(ext2_directory_entry_t);
    char current_name[EXT2_MAX_FILE_NAME + 1];

    do {
        read_data(ext2, parent_inode, offset, size, &entry);

        if (entry.inode == 0)
            return EXT2_ERR_FILE_NOT_FOUND;

        read_data(ext2, parent_inode, offset + size, entry.name_len, current_name);
        current_name[entry.name_len] = '\0';
        offset += entry.rec_len;
    } while (strcmp(current_name, name) != 0);

    return read_inode(ext2, entry.inode, inode);
}

ext2_error_t ext2_open(ext2_t* ext2, const char* path, ext2_file_t* file) {
    ext2_inode_t inode;
    char current_name[EXT2_MAX_FILE_NAME + 1];
    read_inode(ext2, EXT2_ROOT_INODE, &inode);

    uint32_t chars_read;
    while ((chars_read = parse_filename(path, current_name)) > 0) {
        ext2_inode_t next_inode;
        ext2_error_t error = get_directory_entry(ext2, &inode, current_name, &next_inode);
        if (error)
            return error;
        inode = next_inode;
        path += chars_read;
    }

    file->inode = inode;
    return 0;
}

ext2_error_t ext2_mount(ext2_t* ext2, ext2_config_t* cfg) {
    ext2_superblock_t superblk;

    ext2->read = cfg->read;
    ext2->context = cfg->context;

    int superblk_error = read_superblock(ext2, &superblk);

    if (superblk_error < 0)
        return superblk_error;
    
    ext2->superblk = superblk;
    ext2->block_size = 1024 << superblk.log_block_size;

    // if the block size is huge (> 2 GiB), block_size will overflow
    if (ext2->block_size == 0) {
        return EXT2_ERR_BIG_BLOCK;
    }

    return 0; 
}

