#ifndef EXT2_H
#define EXT2_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define EXT2_ROOT_INODE 2
#define EXT2_MAX_FILE_NAME 255
#define SUPERBLOCK_ADDR 1024

// trext2-specific errors. All user-defined errors should be negative (see the
// ext2_config_t struct below)
typedef enum {
    EXT2_ERR_BIG_BLOCK = 1,
    EXT2_ERR_INODE_NOT_FOUND,
    EXT2_ERR_BGD_NOT_FOUND,
    EXT2_ERR_FILENAME_TOO_BIG,
    EXT2_ERR_DATA_OUT_OF_BOUNDS,
    EXT2_ERR_FILE_NOT_FOUND,
    EXT2_ERR_BAD_PATH,
    EXT2_ERR_SEEK_OUT_OF_BOUNDS,
    EXT2_ERR_DISK_FULL,
    EXT2_ERR_NOT_A_FILE,
    EXT2_ERR_NOT_A_DIR,
    EXT2_ERR_INODES_DEPLETED,
} ext2_error_t;

////////////////////////////
/// ext2 disk structures ///
////////////////////////////

// these are the data structures/types that are specified by ext2 and kept 
// inside the disk

// Extracts the 'file format' part of the 'mode' field of an inode
#define GET_FILE_FMT(mode) (0xF000 & (mode))

// file formats present in the upper nibble of inode.mode
typedef enum {
    EXT2_FMT_SOCK = 0xC000,
    EXT2_FMT_LNK  = 0xA000,
    EXT2_FMT_REG  = 0x8000,
    EXT2_FMT_BLK  = 0x6000,
    EXT2_FMT_DIR  = 0x4000,
    EXT2_FMT_CHR  = 0x2000,
    EXT2_FMT_FIFO = 0x1000,
} ext2_file_format_t;

// file formats indicated in the file_type field of dir entries
typedef enum {
    EXT2_ET_UNKNOWN = 0, 
    EXT2_ET_REG,
    EXT2_ET_DIR,
    EXT2_ET_CHR,
    EXT2_ET_BLK,
    EXT2_ET_FIFO,
    EXT2_ET_SOCK,
    EXT2_ET_LNK,
} ext2_dir_ftype_t;

typedef struct {
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t r_blocks_count;
    uint32_t free_blocks_count;
    uint32_t free_inodes_count;
    uint32_t first_data_block;
    uint32_t log_block_size;
    uint32_t log_frag_size;
    uint32_t blocks_per_group;
    uint32_t frags_per_group;
    uint32_t inodes_per_group;
    uint32_t mtime;
    uint32_t wtime;
    uint16_t mnt_count;
    uint16_t max_mnt_count;
    uint16_t magic;
    uint16_t state;
    uint16_t errors;
    uint16_t minor_rev_level;
    uint32_t lastcheck;
    uint32_t checkinterval;
    uint32_t creator_os;
    uint32_t rev_level;
    uint16_t def_resuid;
    uint16_t def_resgid;
} ext2_superblock_t;

typedef struct {
    uint32_t block_bitmap;
    uint32_t inode_bitmap;
    uint32_t inode_table;
    uint16_t free_blocks_count;
    uint16_t free_inodes_count;
    uint16_t used_dirs_count;
    uint16_t pad;
    uint32_t reserved[3];
} ext2_bgd_t; // block group descriptor

typedef struct {
    uint16_t mode;
    uint16_t uid;
    uint32_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint16_t gid;
    uint16_t links_count;
    uint32_t blocks;
    uint32_t flags;
    uint32_t osd1;
    uint32_t block[15];
    uint32_t generation;
    uint32_t file_acl;
    uint32_t dir_acl;
    uint32_t faddr;
    uint32_t osd2[3];
} ext2_inode_t;

typedef struct {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[EXT2_MAX_FILE_NAME + 1];
} ext2_directory_entry_t;

///////////////////////////////
/// t-rext2 data structures ///
///////////////////////////////

// these are the data structures that users of t-rext2 interact with

typedef enum {
    EXT2_FT_UNKNOWN,
    EXT2_FT_DIR,
    EXT2_FT_REGULAR_FILE,
} ext2_file_type_t ;

// configuration used to mount a filesystem
typedef struct {
    // user defined read function. Negative return values will be passed back 
    // to the caller
    int (*read)(uint32_t start, uint32_t size, void* buffer, void* context);

    // user defined write function. Negative return values will be passed back 
    // to the caller
    int (*write)(uint32_t start, uint32_t size, const void* buffer, void* context);

    // this will be passed to the user defined read/write functions, you can
    // put whatever you want here
    void* context;
} ext2_config_t;

// holds state and information about a mounted filesystem
typedef struct {
    int (*read)(uint32_t start, uint32_t size, void* buffer, void* context);
    int (*write)(uint32_t start, uint32_t size, const void* buffer, void* context);

    uint32_t block_size;

    uint32_t block_group_count; // total number of block groups

    void* context;

    ext2_superblock_t superblk;
} ext2_t;

typedef struct {
    uint32_t inode;
    uint32_t offset;
} ext2_file_t;

typedef struct {
    uint32_t inode;
    uint32_t offset;
} ext2_dir_t;

typedef struct {
    uint32_t inode;
    char name[EXT2_MAX_FILE_NAME + 1];
} ext2_dir_record_t;

///////////////////////////////
/// t-rext2 functions       ///
///////////////////////////////

ext2_error_t ext2_mount(ext2_t* ext2, ext2_config_t* cfg);
ext2_error_t ext2_file_open(ext2_t* ext2, const char* path, ext2_file_t* file);
ext2_error_t ext2_file_read(ext2_t* ext2, ext2_file_t* file, uint32_t size, void* buf);
ext2_error_t ext2_file_seek(ext2_t* ext2, ext2_file_t* file, uint32_t offset);
uint32_t ext2_file_tell(ext2_t* ext2, const ext2_file_t* file);
ext2_error_t ext2_file_write(ext2_t* ext2, ext2_file_t* file, uint32_t size, const void* buf);
ext2_error_t ext2_dir_open(ext2_t* ext2, const char* path, ext2_dir_t* dir);
ext2_error_t ext2_dir_read(ext2_t* ext2, ext2_dir_t* dir, ext2_dir_record_t* entry);
ext2_error_t ext2_dir_seek(ext2_t* ext2, ext2_dir_t* dir, uint32_t offset);
uint32_t ext2_dir_tell(ext2_t* ext2, const ext2_dir_t* dir);

// REMOVE THESE LATER
ext2_error_t read_inode(ext2_t* ext2, uint32_t inode_number, ext2_inode_t* inode);
ext2_error_t read_data(ext2_t* ext2, uint32_t inode, uint32_t offset, 
        uint32_t size, void* buffer);
ext2_error_t parse_filename(const char* path, char* filename, uint32_t* chars_read);
#endif
