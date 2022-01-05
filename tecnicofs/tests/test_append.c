#include "../fs/operations.h"
#include <assert.h>
#include <string.h>

#define SIZE 4

int main() {

    char *path = "/f1";

    /* Writing this buffer multiple times to a file stored on 1KB blocks will 
       always hit a single block (since 1KB is a multiple of SIZE=256) */
    char input[SIZE]; 
    memset(input, 'A', SIZE);

    char write_in_append[SIZE];
    memset(write_in_append, 'B', SIZE);

    char output [SIZE];

    assert(tfs_init() != -1);

    /* Write input COUNT times into a new file */
    int fd1 = tfs_open(path, TFS_O_CREAT);
    assert(fd1 != -1);
    /* Write A's */
    assert(tfs_write(fd1, input, SIZE) == SIZE);
    assert(tfs_close(fd1) != -1);

    /* Open with append */
    fd1 = tfs_open(path, TFS_O_APPEND);
    assert(fd1 != -1 );
    /* Open with truncate */
    int fd2 = tfs_open(path, TFS_O_TRUNC);
    assert(fd2 != -1 );

    /* Write B's in append mode */
    assert(tfs_write(fd1, write_in_append, SIZE) == SIZE);
    /* Read B's */
    assert(tfs_read(fd2, output, SIZE) == SIZE);

    assert(memcmp(write_in_append, output, SIZE) == 0);

    assert(tfs_close(fd1) != -1);
    assert(tfs_close(fd2) != -1);



    printf("Sucessful test\n");

    return 0;
}