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
    .write = writeblock,
    .context = DISKIMG_FILE,
};

void prepare_file() {
    FILE *file = fopen(DUMMY_FILE_PATH, "w");
    if (file == NULL) {
        perror("Failed to open file");
        exit(1);
    }

    const uint32_t bpb = BLOCKSZ / sizeof(uint32_t);
    const uint32_t pad = (12 + bpb) * BLOCKSZ;

    // Create a buffer with 'pad' 'a' characters
    // pad is chosen in such a way that the next write to the file
    // will require a double-indirect block to be allocated
    char *buffer = malloc(pad);
    if (buffer == NULL) {
        perror("Failed to allocate buffer");
        fclose(file);
        exit(1);
    }
    memset(buffer, 'a', pad);

    // Write the buffer to the file
    size_t written = fwrite(buffer, 1, pad, file);
    if (written != pad) {
        exit(1);
    }

    free(buffer);
    fclose(file);
}

int main(void) {
    testsuite("big file write");

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

    const uint32_t bpb = BLOCKSZ / sizeof(uint32_t);
    const uint32_t pad = (12 + bpb) * BLOCKSZ;

    ext2_mount(&ext2, &cfg);
    ext2_file_open(&ext2, "/foo/hello", &file);

    char* buf = "Hello world!";
    error = ext2_file_seek(&ext2, &file, pad);

    test("offset is correct", ext2_file_tell(&ext2, &file) == pad);

    error = ext2_file_write(&ext2, &file, strlen(buf), buf);
    
    test("write ok", error == 0);

    ext2_file_seek(&ext2, &file, pad);

    char output_buf[100];
    error = ext2_file_read(&ext2, &file, strlen(buf), output_buf);
    output_buf[strlen(buf)] = '\0';

    test("read ok", error == 0);
    test("written data is correct", strcmp(output_buf, buf) == 0);

    exec_cmd_fail("debugfs -w %s -R \"dump foo/hello /tmp/bye2\"", DISKIMG_FILE);
}
