#include "fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

typedef struct {
    int fhandle;
} tfs_args;

#define SIZE 1024

void *read_t(void *args) {
    char buffer[SIZE+1];
    int *exit_val = (int *) malloc(sizeof(int));
    *exit_val = 0;
    int *fhandle = (int *) args;
    ssize_t read = tfs_read(*fhandle, buffer, SIZE);
    if (read != SIZE) {
        printf("Read failed\n");
        *exit_val = -1;
    }
    buffer[read] = '\0';
    printf("%s\n", buffer);
    pthread_exit((void *) exit_val);
}

void *close_t(void *args){
	int *fhandle = (int *) args;
	int *exit_val = (int *) malloc(sizeof(int));
    *exit_val = 0;
    if (tfs_close(*fhandle) == -1)
    	*exit_val = -1;
    pthread_exit((void *) exit_val);
}

int main() {
    pthread_t tid[3];

    int i = 0;
    int *r[3];
    char *path1 = "/f1";

    char input1[SIZE]; 
    memset(input1, 'A', SIZE);

    char input2[SIZE]; 
    memset(input2, 'B', SIZE);

    int fhandle;
    ssize_t size;

    assert(tfs_init() != -1);

    fhandle = tfs_open(path1, TFS_O_CREAT);
    assert(fhandle != -1);

    size = tfs_write(fhandle, input1, sizeof(input1));
    assert(size == sizeof(input1));

    size = tfs_write(fhandle, input2, sizeof(input2));
    assert(size == sizeof(input2));

    assert(tfs_close(fhandle) != -1);
    fhandle = tfs_open(path1, TFS_O_CREAT);
    assert(fhandle != -1);

    if (pthread_create(&tid[i++], NULL, close_t, (void *)&fhandle) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, read_t, (void *)&fhandle) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, read_t, (void *)&fhandle) != 0) {
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < 3; i++) {
        pthread_join(tid[i], (void **)&r[i]);
        printf("r[%d]: %d\n", i, *r[i]);
    }
    assert(*r[2] != -1);

    printf("Succesful test\n");
    return 0;
}
