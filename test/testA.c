#include "utils.h"
#define DISKIMG_FILE "test/disk1.img"
#define BLOCKSZ 2048
#define BLOCKS_PER_GROUP 256

/************************************* 
 * Filesystem instance and config    *
 *************************************/
ext2_t ext2;
ext2_config_t cfg = {
    .read = readblock,
    .write = writeblock,
    .context = DISKIMG_FILE,
};

int main(void) {
    testsuite("create dir");

    exec_cmd_fail("dd if=/dev/zero of=%s bs=1k count=2048", DISKIMG_FILE);
    exec_cmd_fail("mkfs.ext2 -r 0 -b %d -g %d %s", BLOCKSZ, BLOCKS_PER_GROUP, 
            DISKIMG_FILE);
    exec_cmd_fail("debugfs -w %s -R \"mkdir foo\"", DISKIMG_FILE);

    ext2_mount(&ext2, &cfg);

    ext2_error_t err = ext2_mkdir(&ext2, "/foo/bar");
    
    test("mkdir ok", err == 0);

    ext2_dir_t dir;
    err = ext2_dir_open(&ext2, "/foo/bar", &dir);    

    test("open ok", err == 0);

    ext2_dir_record_t entry;
    err = ext2_dir_read(&ext2, &dir, &entry);

    if (err)
        return err;

    test("1st read name correct", strcmp(entry.name, ".") == 0);
    test("1st read inode correct", entry.inode == dir.inode);

    err = ext2_dir_read(&ext2, &dir, &entry);

    test("2nd read ok", err == 0);
    test("2nd read name correct", strcmp(entry.name, "..") == 0);

    ext2_dir_t parent_dir;
    ext2_dir_open(&ext2, "/foo", &parent_dir);

    test("2nd read inode correct", entry.inode == parent_dir.inode);

    ext2_file_t file;
    ext2_file_open(&ext2, "/foo/hello.txt", &file);
    char* buf = "hi there";
    ext2_file_write(&ext2, &file, strlen(buf), buf);

    err = ext2_mkdir(&ext2, "/bar");

    test("toplevel mkdir ok", err == 0);

    return 0;
}
