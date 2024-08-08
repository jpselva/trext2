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
    .context = DISKIMG_FILE
};

int main(void) {
    testsuite("directory search");

    exec_cmd_fail("dd if=/dev/zero of=%s bs=1k count=2048", DISKIMG_FILE);
    exec_cmd_fail("mkfs.ext2 -r 0 -b %d -g %d %s", BLOCKSZ, BLOCKS_PER_GROUP, 
            DISKIMG_FILE);
    exec_cmd_fail("debugfs -w %s -R \"mkdir foo\"", DISKIMG_FILE);
    exec_cmd_fail("echo \"hello world!\" | tee /tmp/hello");
    exec_cmd_fail("debugfs -w %s -R \"write /tmp/hello foo/hello\"", 
            DISKIMG_FILE);

    ext2_file_t file;

    ext2_mount(&ext2, &cfg);
    ext2_open(&ext2, "foo/hello", &file);

    char buf[100];
    read_data(&ext2, &file.inode, 0, 12, buf);

    return 0;
}
