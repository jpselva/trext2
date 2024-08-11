#include "utils.h"
#define DISKIMG_FILE "test/disk1.img"
#define BLOCKSZ 1024
#define BLOCKS_PER_GROUP 256
#define DUMMY_FILE_PATH "/tmp/hello"

/************************************* 
 * Filesystem instance and config    *
 *************************************/
ext2_t ext2;
ext2_config_t cfg = {
    .read = readblock,
    .context = DISKIMG_FILE
};

void prepare_file() {
    FILE *file = fopen(DUMMY_FILE_PATH, "w");
    if (file == NULL) {
        perror("Failed to open file");
        exit(1);
    }

    // Create a buffer with 600,000 'a' characters
    char *buffer = malloc(600000);
    if (buffer == NULL) {
        perror("Failed to allocate buffer");
        fclose(file);
        exit(1);
    }
    memset(buffer, 'a', 600000);

    // Write the buffer to the file
    size_t written = fwrite(buffer, 1, 600000, file);
    if (written != 600000) {
        exit(1);
    }

    char *hello = "Hello World!";
    size_t len = strlen(hello);
    written = fwrite(hello, 1, len, file);
    if (written != len) {
        perror("Failed to write 'hello world!' to file");
        exit(1);
    }

    // Clean up
    free(buffer);
    fclose(file);
}

int main(void) {
    testsuite("file read");

    exec_cmd_fail("dd if=/dev/zero of=%s bs=1k count=2048", DISKIMG_FILE);
    exec_cmd_fail("mkfs.ext2 -r 0 -b %d -g %d %s", BLOCKSZ, BLOCKS_PER_GROUP, 
            DISKIMG_FILE);
    exec_cmd_fail("debugfs -w %s -R \"mkdir foo\"", DISKIMG_FILE);

    prepare_file();

    // write file
    exec_cmd_fail("debugfs -w %s -R \"write %s foo/hello\"", 
            DISKIMG_FILE, DUMMY_FILE_PATH);

    ext2_file_t file;
    ext2_error_t error;

    ext2_mount(&ext2, &cfg);
    ext2_file_open(&ext2, "/foo/hello", &file);

    char buf[100];
    error = ext2_file_seek(&ext2, &file, 600000);

    test("offset is correct", ext2_file_tell(&ext2, &file) == 600000);

    error = ext2_file_read(&ext2, &file, 12, buf);
    buf[12] = '\0';

    test("read is correct", strcmp(buf, "Hello World!") == 0);
    test("offset changes", ext2_file_tell(&ext2, &file) == 600000 + 12);
}
