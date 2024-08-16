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
    testsuite("create file");

    exec_cmd_fail("dd if=/dev/zero of=%s bs=1k count=2048", DISKIMG_FILE);
    exec_cmd_fail("mkfs.ext2 -r 0 -b %d -g %d %s", BLOCKSZ, BLOCKS_PER_GROUP, 
            DISKIMG_FILE);
    exec_cmd_fail("debugfs -w %s -R \"mkdir foo\"", DISKIMG_FILE);
    exec_cmd_fail("debugfs -w %s -R \"mkdir foo/bar\"", DISKIMG_FILE);

    ext2_mount(&ext2, &cfg);

    ext2_file_t file;
    ext2_error_t err = ext2_file_open(&ext2, "/foo/bar/hello", &file);
    
    test("opens ok", err == 0);

    char* buf = "t-rext2!!!";
    err = ext2_file_write(&ext2, &file, strlen(buf), buf);

    test("writes ok", err == 0);

    ext2_file_t file2;
    err = ext2_file_open(&ext2, "/foo/bar/hello", &file2);

    test("opens ok 2", err == 0);
    
    char buf2[100];
    err = ext2_file_read(&ext2, &file2, strlen(buf), buf2);
    buf2[strlen(buf)] = '\0';

    test("reads ok", err == 0);

    test("data is correct", strcmp(buf2, buf) == 0);

    err = ext2_file_open(&ext2, "/foo/bar/goodbye", &file);

    test("second file created", err == 0);

    char* buf3 = "hi there";
    err = ext2_file_write(&ext2, &file, strlen(buf3), buf3);

    test("second write succeeded", err == 0);

    return 0;
}
