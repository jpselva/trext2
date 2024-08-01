#include "utils.h"

int main() {
    testsuite("test parsepath");

    char buf[EXT2_MAX_FILE_NAME + 1];

    int sz = parse_filename("/foo/bar/hello", buf);

    test("empty file name", strcmp(buf, "") == 0);
    test("size is zero", sz == 0);

    char bigname[EXT2_MAX_FILE_NAME + 1];
    memset(bigname, 'j', EXT2_MAX_FILE_NAME);
    bigname[EXT2_MAX_FILE_NAME] = '\0';

    char bigpath[EXT2_MAX_FILE_NAME + 5];
    bigpath[0] = '\0';
    strncat(bigpath, bigname, EXT2_MAX_FILE_NAME);
    strncat(bigpath, "/foo", 4);

    sz = parse_filename(bigpath, buf);

    test("max file name", strcmp(buf, bigname) == 0);
    test("size is EXT2_MAX_FILE_NAME", sz == EXT2_MAX_FILE_NAME);

    bigpath[EXT2_MAX_FILE_NAME]= '\0';
    strncat(bigpath, "j/hi", 4);

    sz = parse_filename(bigpath, buf);

    test("file name too big", sz == -1);
}
