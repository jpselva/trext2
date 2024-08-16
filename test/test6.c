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
    testsuite("file write");

    exec_cmd_fail("dd if=/dev/zero of=%s bs=1k count=2048", DISKIMG_FILE);
    exec_cmd_fail("mkfs.ext2 -r 0 -b %d -g %d %s", BLOCKSZ, BLOCKS_PER_GROUP, 
            DISKIMG_FILE);
    exec_cmd_fail("debugfs -w %s -R \"mkdir foo\"", DISKIMG_FILE);
    exec_cmd_fail("echo \"hello world!\" | tee /tmp/hello");
    exec_cmd_fail("debugfs -w %s -R \"write /tmp/hello foo/hello\"", 
            DISKIMG_FILE);

    ext2_file_t file;

    ext2_mount(&ext2, &cfg);
    ext2_error_t error = ext2_file_open(&ext2, "/foo/hello", &file);

    char* buf = "bye  ";
    error = ext2_file_write(&ext2, &file, strlen(buf), buf);

    // read file contents with debugfs
    exec_cmd_fail("debugfs -w %s -R \"dump foo/hello /tmp/bye\"", DISKIMG_FILE);
    FILE* output_file = fopen("/tmp/bye", "r");
    char output_buf[100];
    int sz = strlen("bye   world!\n");
    fread(output_buf, sz, 1, output_file);
    output_buf[sz] = '\0';

    test("writes ok", error == 0);

    test("data is correct (debugfs)", strcmp(output_buf, "bye   world!\n") == 0);

    ext2_file_seek(&ext2, &file, 0);
    error = ext2_file_read(&ext2, &file, 13, output_buf);
    output_buf[13] = '\0';

    test("reads ok", error == 0);
    test("data is correct (ext2_read)", strcmp(output_buf, "bye   world!\n") == 0);

    return 0;
}
