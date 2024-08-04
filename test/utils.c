#include "utils.h"

int readblock(uint32_t start, uint32_t size, void* buffer, void* context) {
    FILE* diskfile = fopen((char*) context, "rw");

    if (diskfile == NULL)
        return -1;

    // set read/write position to start of block
    fseek(diskfile, start, SEEK_SET);

    // read 1 string of 'size' elements into buffer
    size_t items_read = fread(buffer, size, 1, diskfile);

    fclose(diskfile);

    return (items_read < 1) ? -1 : 0;
}

int exec_cmd(const char* fmt, ...) {
    char* suffix = " >/dev/null 2>&1";
    char cmd[MAX_CMD_SIZE + 16]; // 16 = suffix size
    va_list args;
    va_start(args, fmt);

    int cmd_size = vsnprintf(cmd, MAX_CMD_SIZE, fmt, args);

    if (cmd_size > MAX_CMD_SIZE) {
        printf("command string is too large");
        exit(1);
    }

    strncat(cmd, suffix, MAX_CMD_SIZE + 16);

    return system(cmd);
}

void exec_cmd_fail(const char* fmt, ...) {
    char* suffix = " >/dev/null 2>&1";
    char cmd[MAX_CMD_SIZE + 16]; // 15 = suffix size
    va_list args;
    va_start(args, fmt);

    int cmd_size = vsnprintf(cmd, MAX_CMD_SIZE, fmt, args);

    if (cmd_size > MAX_CMD_SIZE) {
        perror("command string is too large");
        exit(1);
    }

    strncat(cmd, suffix, MAX_CMD_SIZE + 16);

    if(system(cmd)) {
        printf("command failed: %s", cmd);
        exit(1);
    }
}
