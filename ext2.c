#include "ext2.h"
#define CEIL(x, y) ((x)/(y) + (((x) % (y) == 0) ? 0 : 1))

// blk_indirection_info
//
// explains how a block should be reached from the inode.block array
//
typedef struct {
    int indirection; // from 0 (direct) to 3 (triply-indirect)

    uint32_t indexes[4];
    // 1st element = index in the inode.block array, from 0 to 14
    // 2nd element = index in the 1st indirect block (if indirection > 0)
    // 3rd element = index in the 2nd indirect block (if indirection > 1)
    // 4th element = index in the 3rd indirect block (if indirection > 2)
} blk_indirection_info;

ext2_error_t read_superblock(ext2_t* ext2, ext2_superblock_t* superblk) {
    // superblock starts at 1024th byte
    return ext2->read(SUPERBLOCK_ADDR, sizeof(ext2_superblock_t), 
                      superblk, ext2->context);
}

ext2_error_t write_superblock(ext2_t* ext2, const ext2_superblock_t* superblk) {
    // superblock starts at 1024th byte
    return ext2->write(SUPERBLOCK_ADDR, sizeof(ext2_superblock_t), 
                      superblk, ext2->context);
}

ext2_error_t get_bgd_address(ext2_t* ext2, uint32_t group, uint32_t* addr) {
    if (group >= ext2->block_group_count)
        return EXT2_ERR_BGD_NOT_FOUND;

    // size of a block group descriptor
    uint32_t size = sizeof(ext2_bgd_t);

    // first block of the block group descriptor table
    uint32_t first_block = (ext2->block_size > 1024) ? 1 : 2; 

    // address of the block group descriptor
    *addr = first_block * ext2->block_size + group * size;

    return 0;
}

ext2_error_t read_bgd(ext2_t* ext2, uint32_t group, ext2_bgd_t* bgd) {
    uint32_t addr;
    ext2_error_t err = get_bgd_address(ext2, group, &addr);

    if (err)
        return err;

    return ext2->read(addr, sizeof(ext2_bgd_t), bgd, ext2->context);
}

ext2_error_t write_bgd(ext2_t* ext2, uint32_t group, const ext2_bgd_t* bgd) {
    uint32_t addr;
    ext2_error_t err = get_bgd_address(ext2, group, &addr);

    if (err)
        return err;

    return ext2->write(addr, sizeof(ext2_bgd_t), bgd, ext2->context);
}

uint32_t get_inode_group(ext2_t* ext2, uint32_t inode) {
    uint32_t inodes_per_group = ext2->superblk.inodes_per_group;
    uint32_t block_group = (inode - 1) / inodes_per_group; 
    return block_group;
}

ext2_error_t get_inode_address(ext2_t* ext2, uint32_t inonum, uint32_t* addr) {
    if (inonum > ext2->superblk.inodes_count) {
        return EXT2_ERR_INODE_NOT_FOUND;
    }

    uint32_t inodes_per_group = ext2->superblk.inodes_per_group;
    uint32_t block_group = get_inode_group(ext2, inonum);
    uint32_t offset_inside_group = (inonum - 1) % inodes_per_group;

    ext2_bgd_t bgd;
    ext2_error_t bgd_error = read_bgd(ext2, block_group, &bgd);

    if (bgd_error)
        return bgd_error;

    uint32_t inode_table_addr = bgd.inode_table * ext2->block_size;
    uint32_t inode_size = sizeof(ext2_inode_t);

    *addr= inode_table_addr + offset_inside_group * inode_size;

    return 0;
}

ext2_error_t read_inode(ext2_t* ext2, uint32_t inode_number, ext2_inode_t* inode) {
    uint32_t inode_addr;
    ext2_error_t err = get_inode_address(ext2, inode_number, &inode_addr);

    if (err)
        return err;

    return ext2->read(inode_addr, sizeof(ext2_inode_t), inode, ext2->context);
}

ext2_error_t write_inode(ext2_t* ext2, uint32_t inode_number, const ext2_inode_t* inode) {
    uint32_t inode_addr;
    ext2_error_t err = get_inode_address(ext2, inode_number, &inode_addr);

    if (err)
        return err;

    return ext2->write(inode_addr, sizeof(ext2_inode_t), inode, ext2->context);
}

blk_indirection_info locate_offset(ext2_t* ext2, uint32_t offset) {
    blk_indirection_info ind_info;
    uint32_t blk_index = offset / ext2->block_size;

    if (blk_index < 12) {
        ind_info.indirection = 0;
        ind_info.indexes[0] = blk_index;
        return ind_info;
    }

    blk_index -= 11;

    uint32_t bpb = ext2->block_size / sizeof(uint32_t);
    uint32_t i1 = blk_index % bpb;
    uint32_t i3 = blk_index/(bpb * bpb);
    uint32_t i2 = blk_index/bpb - i3 * bpb;

    int ind = (i3 > 0) ? 3 : (i2 > 0) ? 2 : 1;

    ind_info.indirection = ind;
    ind_info.indexes[0] = 11 + ind;
    ind_info.indexes[1] = (ind == 3) ? i3 - 1 : (ind == 2) ? i2 - 1 : i1 - 1;
    ind_info.indexes[2] = (ind == 3) ? i2 - 1 : i1 - 1;
    ind_info.indexes[3] = i1 - 1;

    return ind_info;
}

ext2_error_t block_map(ext2_t* ext2, uint32_t inode, uint32_t offset, uint32_t* block) {
    ext2_inode_t inode_struct;
    ext2_error_t error = read_inode(ext2, inode, &inode_struct);
    if (error)
        return error;

    blk_indirection_info ind_info = locate_offset(ext2, offset);

    *block = inode_struct.block[ind_info.indexes[0]];

    for(int i = 1; i <= ind_info.indirection; i++) {
        if (*block == 0) // block 0 implies data block is all zeros, so stop here
            break;

        uint32_t addr = *block * ext2->block_size + ind_info.indexes[i] * sizeof(uint32_t);

        error = ext2->read(addr, sizeof(uint32_t), block, ext2->context);
        if (error)
            return error;
    }

    return 0;
}

ext2_error_t read_data(ext2_t* ext2, uint32_t inode, uint32_t offset, 
        uint32_t size, void* buffer) {
    ext2_inode_t inode_struct;

    ext2_error_t error = read_inode(ext2, inode, &inode_struct);
    if (error)
        return error;

    if (offset + size > inode_struct.size) {
        // access is outside of file
        return EXT2_ERR_DATA_OUT_OF_BOUNDS;
    }

    while (size > 0) {
        uint32_t datablock;

        error = block_map(ext2, inode, offset, &datablock);
        if (error)
            return error;

        uint32_t block_offset = offset % ext2->block_size;
        uint32_t remaining = ext2->block_size - block_offset;
        uint32_t bytes_to_read = (remaining < size) ? remaining : size;

        ext2_error_t error;

        if (datablock == 0) {
            memset(buffer, 0, bytes_to_read);
        } else {
            error = ext2->read(datablock * ext2->block_size + block_offset,  
                bytes_to_read, (uint8_t*)buffer, ext2->context);
        }

        if (error)
            return error;

        buffer = (uint8_t*)buffer + bytes_to_read;
        offset += bytes_to_read;
        size -= bytes_to_read;
    }

    return 0;
}

// extracts the first name before the '/' in a file path. filename should have
// at least EXT2_MAX_FILE_NAME + 1 bytes. 
ext2_error_t parse_filename(const char* path, char* filename, uint32_t* chars_read) {
    int i = 0;
    char c = *(path++);

    while (c != '\0' && c != '/') {
        if (i >= EXT2_MAX_FILE_NAME) {
            return EXT2_ERR_FILENAME_TOO_BIG;
        }

        filename[i++] = c;
        c = *(path++);
    }

    filename[i] = '\0';

    *chars_read = i;
    return 0;
}

ext2_error_t read_dir_entry(ext2_t* ext2, uint32_t offset, 
        uint32_t dir_inode, ext2_directory_entry_t* entry) {
    ext2_inode_t inode_struct;

    ext2_error_t error = read_inode(ext2, dir_inode, &inode_struct);
    if (error)
        return error;

    if (GET_FILE_FMT(inode_struct.mode) != EXT2_FMT_DIR)
        return EXT2_ERR_NOT_A_DIR;

    // total size of the fields of the dir entry struct that precede the name
    // of the file
    uint32_t headersz = (uint8_t*)&entry->name - (uint8_t*)entry;

    error = read_data(ext2, dir_inode, offset, headersz, entry);
    if (error)
        return error;

    error = read_data(ext2, dir_inode, offset + headersz, entry->name_len, 
            &entry->name);
    if (error)
        return error;

    entry->name[entry->name_len] = '\0';
    return 0;
}

ext2_error_t locate_inode_in_dir(ext2_t* ext2, uint32_t parent_inode, 
        const char* name, uint32_t* inode) {
    ext2_directory_entry_t entry;
    uint32_t offset = 0;

    ext2_inode_t inode_struct;
    ext2_error_t error = read_inode(ext2, parent_inode, &inode_struct);

    if (error)
        return error;

    do {
        error = read_dir_entry(ext2, offset, parent_inode, &entry);

        if (error)
            return error;

        if (strcmp(entry.name, name) == 0) {
            *inode = entry.inode;
            return 0;
        }

        offset += entry.rec_len;
    } while (offset < inode_struct.size);

    return EXT2_ERR_FILE_NOT_FOUND;
}

// Reserves a block inside a group. Sets 'block' to the first free block inside 
// group or 0 if group has no free blocks. 
ext2_error_t get_free_block_in_group(ext2_t* ext2, uint32_t group, 
        uint32_t* block) {
    ext2_bgd_t bgd;
    ext2_error_t error = read_bgd(ext2, group, &bgd);

    if (error)
        return error;

    const uint32_t bitmap_addr = bgd.block_bitmap * ext2->block_size;
    const uint32_t blocks_per_group = ext2->superblk.blocks_per_group;
    const uint32_t inode_table_size = ext2->superblk.inodes_per_group * 
        sizeof(ext2_inode_t);
    const uint32_t inode_table_block_count = inode_table_size / ext2->block_size + 
        ((inode_table_size % ext2->block_size == 0) ? 0 : 1);
    const uint32_t data_blocks_start = bgd.inode_table + inode_table_block_count;

    *block = 0; // no free block found yet

    if (bgd.free_blocks_count == 0)
        return 0;

    for (int i = 0; i < blocks_per_group/8; i++) {
        uint8_t bitmap_byte;
        error = ext2->read(bitmap_addr + i*8, 1, &bitmap_byte, ext2->context);

        if (error)
            return error;

        bool found_block = false;
        int shift;

        for (shift = 0; shift < 8; shift++) {
            found_block = ((bitmap_byte & (1 << shift)) == 0);
            if (found_block)
                break;
        }

        if (found_block) {
            *block = i * 8 + shift + data_blocks_start;

            bitmap_byte |= (1 << shift);
            ext2->write(bitmap_addr + i*8, 1, &bitmap_byte, ext2->context);

            bgd.free_blocks_count -= 1;
            write_bgd(ext2, group, &bgd);

            ext2->superblk.free_blocks_count -= 1;
            write_superblock(ext2, &ext2->superblk);

            break;
        }
    }

    return 0;
}

ext2_error_t get_new_block(ext2_t* ext2, uint32_t preferred_group, 
        uint32_t* block) {
    int group = preferred_group;
    do {
        ext2_error_t err = get_free_block_in_group(ext2, group, block);

        if (err)
            return err;

        if (*block != 0)
            return 0;

        group = (group + 1) % ext2->block_group_count;
    } while (group != preferred_group);

    return EXT2_ERR_DISK_FULL;
}

ext2_error_t add_block(ext2_t* ext2, uint32_t inode, uint32_t* block) {
    ext2_inode_t inode_struct;
    uint32_t inode_addr;
    uint32_t group;
    ext2_error_t err;

    err = get_inode_address(ext2, inode, &inode_addr);
    if (err) 
        return err;
    err = read_inode(ext2, inode, &inode_struct);
    group = get_inode_group(ext2, inode);

    uint32_t offset = inode_struct.size + ext2->block_size - 1;
    blk_indirection_info ind_info = locate_offset(ext2, offset);

    uint32_t block_ptr;

    block_ptr = ((uint8_t*)(&inode_struct.block[ind_info.indexes[0]]) - 
            (uint8_t*)&inode_struct) + inode_addr;

    uint32_t next_block;

    for (int i = 1; i <= ind_info.indirection; i++) {
        bool is_blk_missing = true;
        for (int j = i; j <= ind_info.indirection; j++)
            is_blk_missing = is_blk_missing && (ind_info.indexes[j] == 0);

        if (is_blk_missing) {
            err = get_new_block(ext2, group, &next_block);

            if (err)
                return err;

            ext2->write(block_ptr, 4, &next_block, ext2->context);
        } else {
            ext2->read(block_ptr, 4, &next_block, ext2->context);
        }

        block_ptr = next_block * ext2->block_size + 
            ind_info.indexes[i] * sizeof(uint32_t);
    }

    err = get_new_block(ext2, group, block);

    if (err)
        return err;

    ext2->write(block_ptr, 4, block, ext2->context);
    return 0;
}

ext2_error_t  write_data(ext2_t* ext2, uint32_t inode, uint32_t offset,
        uint32_t size, const void* buffer) {
    ext2_error_t error;
    ext2_inode_t inode_struct;

    error = read_inode(ext2, inode, &inode_struct);

    if (error)
        return error;

    if (offset > inode_struct.size)
        return EXT2_ERR_DATA_OUT_OF_BOUNDS;

    uint32_t allocated_size = CEIL(inode_struct.size, ext2->block_size) * 
                                  ext2->block_size;

    uint32_t sz = size;
    while (sz > 0) {
        uint32_t datablock;

        if (offset >= allocated_size) {
            error = add_block(ext2, inode, &datablock);
        } else {
            error = block_map(ext2, inode, offset, &datablock);  
        }

        if (error)
            return error;

        uint32_t block_offset = offset % ext2->block_size;
        uint32_t remaining = ext2->block_size - block_offset;
        uint32_t bytes_to_write = (remaining < sz) ? remaining : sz;

        // TODO: write 0 to inode block number if block is all zeros
        error = ext2->write(datablock * ext2->block_size + block_offset,
            bytes_to_write, buffer, ext2->context);

        if (error)
            return error;

        buffer = (uint8_t*)buffer + bytes_to_write;
        offset += bytes_to_write;
        sz -= bytes_to_write;
    }

    // re-read inode because it might have changed
    error = read_inode(ext2, inode, &inode_struct);
    inode_struct.size += size;
    write_inode(ext2, inode, &inode_struct);

    return 0;
}

ext2_error_t locate_inode(ext2_t* ext2, const char* path, uint32_t* inode) {
    uint32_t ino = EXT2_ROOT_INODE;
    char current_name[EXT2_MAX_FILE_NAME + 1];

    uint32_t chars_read;
    ext2_error_t error;

    if (path[0] != '/') {
        return EXT2_ERR_BAD_PATH;
    } else if (path[1] != '\0') {
        while (*path == '/') {
            path++;

            error = parse_filename(path, current_name, &chars_read);

            if (error)
                return error;

            error = locate_inode_in_dir(ext2, ino, current_name, &ino);

            if (error)
                return error;

            path += chars_read;
        }
    }

    *inode = ino;

    return 0;
}

ext2_error_t locate_parent_inode(ext2_t* ext2, const char* path, uint32_t* parent) {
    uint32_t ino = EXT2_ROOT_INODE;
    uint32_t previous_ino = ino;
    char current_name[EXT2_MAX_FILE_NAME + 1];

    uint32_t chars_read;
    ext2_error_t error;

    if (path[0] != '/') {
        return EXT2_ERR_BAD_PATH;
    } else if (path[1] != '\0') {
        while (*path == '/') {
            path++;

            error = parse_filename(path, current_name, &chars_read);

            if (error)
                return error;

            path += chars_read;

            previous_ino = ino;

            if (*path == '\0')
                break;

            error = locate_inode_in_dir(ext2, ino, current_name, &ino);

            if (error)
                return error;

        }
    }

    *parent = previous_ino;
    return 0;
}

ext2_error_t get_free_inode_in_group(ext2_t* ext2, uint32_t group, uint32_t* inode) {
    ext2_bgd_t bgd;
    ext2_error_t error = read_bgd(ext2, group, &bgd);

    if (error)
        return error;

    uint32_t bitmap_addr = bgd.inode_bitmap * ext2->block_size;
    uint32_t inode_table_addr = bgd.inode_table * ext2->block_size;
    uint32_t first_inode = group * ext2->superblk.inodes_per_group + 1;
    *inode = 0; // no free inode found yet

    if (bgd.free_inodes_count == 0)
        return 0;

    for (int i = 0; i < ext2->superblk.inodes_per_group/8; i++) {
        uint8_t bitmap_byte;
        error = ext2->read(bitmap_addr + i*8, 1, &bitmap_byte, ext2->context);

        if (error)
            return error;

        bool found_inode = false;
        int shift;

        for (shift = 0; shift < 8; shift++) {
            found_inode = ((bitmap_byte & (1 << shift)) == 0);
            if (found_inode)
                break;
        }

        if (found_inode) {
            *inode = i * 8 + shift + first_inode;

            bitmap_byte |= (1 << shift);
            ext2->write(bitmap_addr + i*8, 1, &bitmap_byte, ext2->context);

            bgd.free_inodes_count -= 1;
            write_bgd(ext2, group, &bgd);

            ext2->superblk.free_inodes_count -= 1;
            write_superblock(ext2, &ext2->superblk);

            break;
        }
    }

    return 0;
}

ext2_error_t get_free_inode(ext2_t* ext2, uint32_t preferred_group, 
        uint32_t* inode) {
    int group = preferred_group;

    do {
        ext2_error_t err = get_free_inode_in_group(ext2, group, inode);

        if (err)
            return err;

        if (*inode != 0)
            return 0;

        group = (group + 1) % ext2->block_group_count;
    } while (group != preferred_group);

    return EXT2_ERR_INODES_DEPLETED;
}

ext2_error_t create_dir_entry(ext2_t* ext2, uint32_t inode, const char* name,
        ext2_directory_entry_t* entry) {
    uint32_t namelen = strlen(name);
    if (namelen > EXT2_MAX_FILE_NAME)
        return EXT2_ERR_FILENAME_TOO_BIG;

    uint32_t headersz = ((uint8_t*)&entry->name - (uint8_t*)entry);

    entry->inode = inode;
    entry->name_len = namelen;
    entry->file_type = 0;
    strcpy(entry->name, name);

    return 0;
}

ext2_error_t get_last_dir_entry_offset(ext2_t* ext2, uint32_t dir_inode, 
        uint32_t* offset) {

    ext2_inode_t inode_struct;
    ext2_error_t err = read_inode(ext2, dir_inode, &inode_struct);
    if (err)
        return err;

    // obtain the last directory entry
    ext2_directory_entry_t last_entry;
    uint32_t off, last_entry_off = -1;

    for (off = 0; off < inode_struct.size; off += last_entry.rec_len) {
        err = read_dir_entry(ext2, off, dir_inode, &last_entry);
        if (err)
            return err;
        last_entry_off = off;
    }

    *offset = last_entry_off;
    return 0;
}

uint32_t get_dir_entry_size(const ext2_directory_entry_t* entry) {
    uint32_t headersz = ((uint8_t*)&entry->name - (uint8_t*)entry);
    return headersz + entry->name_len;
}

ext2_error_t add_dir_entry(ext2_t* ext2, uint32_t dir_inode, 
        ext2_directory_entry_t* entry) {

    ext2_inode_t inode_struct;
    ext2_error_t err = read_inode(ext2, dir_inode, &inode_struct);
    if (err)
        return err;

    // get last entry in the directory
    uint32_t last_entry_offset;
    ext2_directory_entry_t last_entry;
    err = get_last_dir_entry_offset(ext2, dir_inode, &last_entry_offset);
    if (err)
        return err;

    bool is_first_entry = (last_entry_offset == -1);

    uint32_t new_entry_offset;

    if (is_first_entry) {
        new_entry_offset = 0;
    } else {
        err = read_dir_entry(ext2, last_entry_offset, dir_inode, &last_entry);
        if (err)
            return err;

        new_entry_offset = last_entry_offset + get_dir_entry_size(&last_entry);
    }

    // align address
    new_entry_offset = CEIL(new_entry_offset, 4) * 4; // 4-byte aligned

    // make sure new entry won't span two blocks
    uint32_t starting_block = new_entry_offset / ext2->block_size;
    uint32_t entry_size = get_dir_entry_size(entry);
    uint32_t ending_block = (new_entry_offset + entry_size) / ext2->block_size;
    if (starting_block < ending_block)
        // push everything to the next block
        new_entry_offset = ending_block * ext2->block_size;

    // add padding if needed
    if (!is_first_entry) {
        int off = last_entry_offset + get_dir_entry_size(&last_entry);
        char pad = '0';
        while (off < new_entry_offset) {
            write_data(ext2, dir_inode, off, 1, &pad);
            off++;
        }
    }

    // write next entry
    entry->rec_len = (ending_block + 1) * ext2->block_size - new_entry_offset;
    write_data(ext2, dir_inode, new_entry_offset, get_dir_entry_size(entry), 
            entry);

    if (!is_first_entry) {
        // update previous last entry
        last_entry.rec_len = new_entry_offset - last_entry_offset;
        err = write_data(ext2, dir_inode, last_entry_offset, 
                get_dir_entry_size(&last_entry), &last_entry);
        if (err)
            return err;
    }

    return 0;
}

ext2_error_t link(ext2_t* ext2, uint32_t dir_inode, uint32_t inode, 
        const char* name) {

    ext2_directory_entry_t entry;
    ext2_error_t err = create_dir_entry(ext2, inode, name, &entry);
    if (err)
        return err;

    err = add_dir_entry(ext2, dir_inode, &entry);
    if (err)
        return err;

    // TODO: update inode link count

    return 0;
}

// extracts the filename from a full path
ext2_error_t get_file_name(const char* path, char* name) {
    const char* lastsep = path;
    while (*path != '\0') {
        if (*path == '/')
            lastsep = path + 1;
        path++;
    }
    
    if (strlen(lastsep) > EXT2_MAX_FILE_NAME)
        return EXT2_ERR_FILENAME_TOO_BIG;

    strcpy(name, lastsep);

    return 0;
}

// creates a new inode and links the path to it. Stores the new inode number in
// the 'ino' pointer
ext2_error_t create_and_link_inode(ext2_t* ext2, const char* path, uint32_t* ino) {
    // locate parent dir inode
    uint32_t dir_inode;
    ext2_error_t err = locate_parent_inode(ext2, path, &dir_inode);
    if (err)
        return err;

    // allocate a new inode for the file
    uint32_t inode;
    err = get_free_inode(ext2, get_inode_group(ext2, dir_inode), &inode);
    if (err)
        return err;

    // link inode to the desired file
    char filename[EXT2_MAX_FILE_NAME + 1];
    err = get_file_name(path, filename);
    if (err)
        return err;
    err = link(ext2, dir_inode, inode, filename);
    if (err)
        return err;

    *ino = inode;
    return 0;
}

void set_inode_file_fmt(ext2_inode_t* inode_struct, ext2_file_format_t fmt) {
    inode_struct->mode &= 0x0FFF;
    inode_struct->mode |= fmt;
}

ext2_error_t ext2_mount(ext2_t* ext2, ext2_config_t* cfg) {
    ext2_superblock_t superblk;

    ext2->read = cfg->read;
    ext2->write = cfg->write;
    ext2->context = cfg->context;

    int superblk_error = read_superblock(ext2, &superblk);

    if (superblk_error)
        return superblk_error;
    
    ext2->superblk = superblk;
    ext2->block_size = 1024 << superblk.log_block_size;
    ext2->block_group_count = superblk.inodes_count / superblk.inodes_per_group;

    // if the block size is huge (> 2 GiB), block_size will overflow
    if (ext2->block_size == 0) {
        return EXT2_ERR_BIG_BLOCK;
    }

    return 0; 
}

ext2_error_t ext2_file_open(ext2_t* ext2, const char* path, ext2_file_t* file) {
    uint32_t inode;
    ext2_error_t error;
    ext2_inode_t inode_struct;

    error = locate_inode(ext2, path, &inode);

    if (error == EXT2_ERR_FILE_NOT_FOUND) {
        error = create_and_link_inode(ext2, path, &inode);
        if (error)
            return error;

        error = read_inode(ext2, inode, &inode_struct);
        if (error)
            return error;

        set_inode_file_fmt(&inode_struct, EXT2_FMT_REG);
        error = write_inode(ext2, inode, &inode_struct);
        if (error)
            return error;
    }

    if (error)
        return error;

    error = read_inode(ext2, inode, &inode_struct);

    if (error)
        return error;

    // ensure inode actually refers to a file
    if (GET_FILE_FMT(inode_struct.mode) != EXT2_FMT_REG)
        return EXT2_ERR_NOT_A_FILE;

    file->inode = inode;
    file->offset = 0;
    return 0;
}

ext2_error_t ext2_file_read(ext2_t* ext2, ext2_file_t* file, 
        uint32_t size, void* buf) {
    ext2_error_t error = read_data(ext2, file->inode, file->offset, size, buf);

    if (!error)
        file->offset += size;

    return error;
}

ext2_error_t ext2_file_seek(ext2_t* ext2, ext2_file_t* file, uint32_t offset) {
    ext2_inode_t inode_struct;
    ext2_error_t error = read_inode(ext2, file->inode, &inode_struct);
    if (error)
        return error;

    if (offset > inode_struct.size) {
        return EXT2_ERR_SEEK_OUT_OF_BOUNDS;
    }

    file->offset = offset;
    return 0;
}

uint32_t ext2_file_tell(ext2_t* ext2, const ext2_file_t* file) {
    return file->offset;
}

ext2_error_t ext2_file_write(ext2_t* ext2, ext2_file_t* file, 
        uint32_t size, const void* buf) {
    ext2_error_t error = write_data(ext2, file->inode, file->offset, size, buf);

    if (!error)
        file->offset += size;

    return error;
}

ext2_error_t ext2_dir_open(ext2_t* ext2, const char* path, ext2_dir_t* dir) {
    uint32_t inode;
    ext2_error_t error;

    error = locate_inode(ext2, path, &inode);

    if (error)
        return error;

    ext2_inode_t inode_struct;
    error = read_inode(ext2, inode, &inode_struct);

    if (error)
        return error;

    // ensure inode actually refers to a dir
    if (GET_FILE_FMT(inode_struct.mode) != EXT2_FMT_DIR)
        return EXT2_ERR_NOT_A_DIR;

    dir->inode = inode;
    dir->offset = 0;
    return 0;
}

ext2_error_t ext2_mkdir(ext2_t* ext2, const char* path) {
    uint32_t inode;
    ext2_inode_t inode_struct;
    ext2_error_t error = create_and_link_inode(ext2, path, &inode);
    if (error)
        return error;

    error = read_inode(ext2, inode, &inode_struct);
    if (error)
        return error;

    set_inode_file_fmt(&inode_struct, EXT2_FMT_DIR);
    error = write_inode(ext2, inode, &inode_struct);
    if (error)
        return error;

    uint32_t parent_inode;
    locate_parent_inode(ext2, path, &parent_inode);

    ext2_directory_entry_t dot;
    error = create_dir_entry(ext2, inode, ".", &dot);
    if (error)
        return error;

    ext2_directory_entry_t dotdot;
    error = create_dir_entry(ext2, parent_inode, "..", &dotdot);
    if (error)
        return error;

    error = add_dir_entry(ext2, inode, &dot);
    if (error)
        return error;

    error = add_dir_entry(ext2, inode, &dotdot);
    if (error)
        return error;

    return 0;
}

ext2_error_t ext2_dir_read(ext2_t* ext2, ext2_dir_t* dir, ext2_dir_record_t* entry) {
    ext2_inode_t inode_struct;
    ext2_error_t error = read_inode(ext2, dir->inode, &inode_struct);
    if (error)
        return error;

    if (dir->offset == inode_struct.size) { // already read all entries
        // subsequent reads return an empty name and inode 0
        entry->inode = 0;
        entry->name[0] = '\0'; 
        return 0;
    }

    ext2_directory_entry_t disk_entry;
    error = read_dir_entry(ext2, dir->offset, dir->inode, &disk_entry);
    if (error)
        return error;

    entry->inode = disk_entry.inode;
    strcpy(entry->name, disk_entry.name);
    dir->offset += disk_entry.rec_len;
    return 0;
}

ext2_error_t ext2_dir_seek(ext2_t* ext2, ext2_dir_t* dir, uint32_t offset) {
    ext2_inode_t inode_struct;
    ext2_error_t error = read_inode(ext2, dir->inode, &inode_struct);
    if (error)
        return error;

    if (offset > inode_struct.size) {
        return EXT2_ERR_SEEK_OUT_OF_BOUNDS;
    }

    dir->offset = offset;
    return 0;
}

uint32_t ext2_dir_tell(ext2_t* ext2, const ext2_dir_t* dir) {
    return dir->offset;
}
