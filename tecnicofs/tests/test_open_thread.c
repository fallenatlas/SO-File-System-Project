#include "fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>

typedef struct {
    char const *path;
    int flag;
} tfs_open_args;

typedef struct {
    int fhandle;
    char *str;
    size_t to_write;
    int tid;
} tfs_write_args;

typedef struct {
    int fhandle;
    char *str;
    size_t to_read;
} tfs_read_args;

typedef struct {
    int fhandle;
} tfs_close_args;

void *open1(void *args) {
    int fhandle;
    int *exit_val = (int *) malloc(sizeof(int));
    *exit_val = 0;
    tfs_open_args *open_args = (tfs_open_args *) args;
    fhandle = tfs_open(open_args->path, open_args->flag);
    printf("fhandle: %d\n", fhandle);
    if (fhandle == -1) {
        printf("Open failed\n");
        *exit_val = -1;
        pthread_exit((void *) exit_val);
    }
    printf("Closing file: %s, fhandle: %d\n", open_args->path, fhandle);
    if (tfs_close(fhandle) == -1) {
        printf("Close failed\n");
        *exit_val = -1;
        pthread_exit((void *) exit_val);
    }
    printf("File closed: %s, fhandle: %d\n", open_args->path, fhandle);
    pthread_exit((void *) exit_val);
}

int main() {

    pthread_t tid[3];

    int i;
    int *r[3];
    char *path1 = "/f1";
    char *path2 = "/f2";

    assert(tfs_init() != -1);

    tfs_open_args *open_args_1 = (tfs_open_args*) malloc(sizeof(tfs_open_args));
    open_args_1->path = path1;
    open_args_1->flag = TFS_O_CREAT;
    tfs_open_args *open_args_2 = (tfs_open_args*) malloc(sizeof(tfs_open_args));
    open_args_2->path = path2;
    open_args_2->flag = TFS_O_CREAT;

    for(i = 0; i < 2; i++) {
        if (pthread_create(&tid[i], NULL, open1, (void *)open_args_1) != 0) {
            exit(EXIT_FAILURE);
        }
    }
    if (pthread_create(&tid[i], NULL, open1, (void *)open_args_2) != 0) {
        exit(EXIT_FAILURE);
    }
    
    for (i = 0; i < 3; i++) {
        pthread_join(tid[i], (void **)&r[i]);
        printf("r[%d]: %d\n", i, *r[i]);
        assert(*r[i] != -1);
    }

    printf("Successful test.\n");

    return 0;
}