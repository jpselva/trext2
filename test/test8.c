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
    testsuite("dir read");

    exec_cmd_fail("dd if=/dev/zero of=%s bs=1k count=2048", DISKIMG_FILE);
    exec_cmd_fail("mkfs.ext2 -r 0 -b %d -g %d %s", BLOCKSZ, BLOCKS_PER_GROUP, 
            DISKIMG_FILE);
    exec_cmd_fail("debugfs -w %s -R \"mkdir foo\"", DISKIMG_FILE);
    exec_cmd_fail("debugfs -w %s -R \"mkdir foo/bar\"", DISKIMG_FILE);
    exec_cmd_fail("echo \"hello world!\" | tee /tmp/hello");
    exec_cmd_fail("debugfs -w %s -R \"write /tmp/hello foo/hello\"", 
            DISKIMG_FILE);

    ext2_dir_t dir;

    ext2_mount(&ext2, &cfg);
    ext2_error_t error = ext2_dir_open(&ext2, "/foo", &dir);

    test("opens ok", error == 0);

    ext2_dir_record_t entry;

    error = ext2_dir_read(&ext2, &dir, &entry);

    test("reads ok", error == 0);
    test("1st entry has right name", strcmp(entry.name, ".") == 0);

    error = ext2_dir_read(&ext2, &dir, &entry);
    test("2nd entry has right name", strcmp(entry.name, "..") == 0);

    uint32_t offset = ext2_dir_tell(&ext2, &dir);

    error = ext2_dir_read(&ext2, &dir, &entry);
    test("3rd entry has right name", strcmp(entry.name, "bar") == 0);

    error = ext2_dir_read(&ext2, &dir, &entry);
    test("4th entry has right name", strcmp(entry.name, "hello") == 0);

    error = ext2_dir_read(&ext2, &dir, &entry);
    test("last entry has empty name", strcmp(entry.name, "") == 0);

    error = ext2_dir_read(&ext2, &dir, &entry);
    test("last entry has empty name (again)", strcmp(entry.name, "") == 0);

    error = ext2_dir_seek(&ext2, &dir, offset);
    test("seek ok", error == 0);

    error = ext2_dir_read(&ext2, &dir, &entry);
    test("3rd entry has right name (again)", strcmp(entry.name, "bar") == 0);

    return 0;
}
