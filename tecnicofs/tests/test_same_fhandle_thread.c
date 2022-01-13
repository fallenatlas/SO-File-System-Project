#include "fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

typedef struct {
    int fhandle;
} tfs_args;

#define SIZE 1024

/* Test readings and writings in the same file handle. 
After writing a block of 'A', a block of 'B' and a block of 'C',
3 threads are created (file handle with offset starting in 0):
- 2 are going to read a block
- 1 is going to overwrite a block
Output expected:  when printing what the 2 threads read, a block of the same letter
should be seen (a block of 'A', 'B' or 'C'), the 2 prints should not return the same block
nor the block 'D', since the same file handle is used. */

/* Read a block */
void *read_t(void *args) {
    char buffer[SIZE+1];
    int *fhandle = (int *) args;
    ssize_t read = tfs_read(*fhandle, buffer, SIZE);

    assert(read == SIZE);
    buffer[read] = '\0';
    /* Print what was read */
    printf("Read: %s\n", buffer);
    return NULL;
}

/* Write a block of 'D' */
void *write_t(void *args) {
    int *fhandle = (int *) args;
    char to_write[SIZE];

    memset(to_write, 'D', SIZE);
    ssize_t written = tfs_write(*fhandle, to_write, SIZE);

    assert(written == SIZE);
    return NULL;
}

int main() {
    pthread_t tid[3];

    int i = 0;
    char *path1 = "/f1";

    char input1[SIZE]; 
    memset(input1, 'A', SIZE);

    char input2[SIZE]; 
    memset(input2, 'B', SIZE);

    char input3[SIZE];
    memset(input3, 'C', SIZE);

    int fhandle;
    ssize_t size;

    assert(tfs_init() != -1);
    /* Create a new file and write a block of 'A', a block of 'B'
    and a block of 'C' */
    fhandle = tfs_open(path1, TFS_O_CREAT);
    assert(fhandle != -1);

    size = tfs_write(fhandle, input1, sizeof(input1));
    assert(size == sizeof(input1));

    size = tfs_write(fhandle, input2, sizeof(input2));
    assert(size == sizeof(input2));

    size = tfs_write(fhandle, input3, sizeof(input3));
    assert(size == sizeof(input3));

    assert(tfs_close(fhandle) != -1);
    fhandle = tfs_open(path1, TFS_O_CREAT);
    assert(fhandle != -1);
    /* Create threads that will try to read from the same file handle */
    if (pthread_create(&tid[i++], NULL, read_t, (void *)&fhandle) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, read_t, (void *)&fhandle) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, write_t, (void *)&fhandle) != 0) {
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < 3; i++) {
        pthread_join(tid[i], NULL);
    }

    printf("Successful test\n");
    return 0;
}
