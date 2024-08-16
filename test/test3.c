#include "utils.h"

ext2_error_t parse_filename(const char* path, char* filename, uint32_t* chars_read);

int main() {
    testsuite("test parsepath");

    char buf[EXT2_MAX_FILE_NAME + 1];

    uint32_t sz;
    ext2_error_t error = parse_filename("foo/bar/hello", buf, &sz);

    test("regular file name", strcmp(buf, "foo") == 0);
    test("no errors", error == 0);
    test("size is correct", sz == 3);

    char bigname[EXT2_MAX_FILE_NAME + 1];
    memset(bigname, 'j', EXT2_MAX_FILE_NAME);
    bigname[EXT2_MAX_FILE_NAME] = '\0';

    char bigpath[EXT2_MAX_FILE_NAME + 5];
    bigpath[0] = '\0';
    strncat(bigpath, bigname, EXT2_MAX_FILE_NAME);
    strncat(bigpath, "/foo", 4);

    error = parse_filename(bigpath, buf, &sz);

    test("max file name", strcmp(buf, bigname) == 0);
    test("no errors", error == 0);
    test("size is EXT2_MAX_FILE_NAME", sz == EXT2_MAX_FILE_NAME);

    bigpath[EXT2_MAX_FILE_NAME]= '\0';
    strncat(bigpath, "j/hi", 4);

    error = parse_filename(bigpath, buf, &sz);

    test("file name too big", error == EXT2_ERR_FILENAME_TOO_BIG);
}
