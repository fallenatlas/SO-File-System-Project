#include "fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

typedef struct {
    char const *path;
    char const *path2;
} tfs_args_copy;

typedef struct {
    char const *path;
    int flag;
    char *str;
    size_t to_write;
} tfs_args_write;

#define SIZE 1024
#define FILE_SIZE 3072

void* copy(void* args) {
    int *exit_val = (int *) malloc(sizeof(int));
    *exit_val = 0;
    tfs_args_copy* _args = (tfs_args_copy *) args;
    char to_read[FILE_SIZE];
    if (tfs_copy_to_external_fs(_args->path, _args->path2) == -1)
        *exit_val = -1;

    FILE *fp = fopen(_args->path2, "r");
    assert(fp != NULL);
    size_t read = fread(to_read, sizeof(char), FILE_SIZE, fp);
    to_read[read] = '\0';
    printf("%s\n", to_read);
    assert(fclose(fp) != -1);
    unlink(_args->path2);
    pthread_exit((void*) exit_val);
}

void* write_t(void* args) {
    int *exit_val = (int *) malloc(sizeof(int));
    *exit_val = 0;
    tfs_args_write* _args = (tfs_args_write*) args;
    int fhandle = tfs_open(_args->path, _args->flag);
    if (fhandle == -1) {
        *exit_val = -1;
        printf("Open failed\n");
        pthread_exit((void*) exit_val);
    }
    if (tfs_write(fhandle, _args->str, _args->to_write) != _args->to_write) {
        printf("Write failed\n");
        *exit_val = -1;
    }
    if (tfs_close(fhandle) == -1) {
        printf("Close failed\n");
        *exit_val = -1;
    }
    pthread_exit((void*) exit_val);
}

int main() {
    pthread_t tid[2];

    int i = 0;
    int *r[2];
    char *path = "/f1";
    char *path2 = "external_file.txt";

    char input1[SIZE]; 
    memset(input1, 'A', SIZE);

    char input2[SIZE]; 
    memset(input2, 'B', SIZE);

    int fhandle;
    ssize_t size;

    assert(tfs_init() != -1);

    fhandle = tfs_open(path, TFS_O_CREAT);
    assert(fhandle != -1);

    size = tfs_write(fhandle, input1, sizeof(input1));
    assert(size == sizeof(input1));

    size = tfs_write(fhandle, input2, sizeof(input2));
    assert(size == sizeof(input2));

    tfs_args_copy* args_copy = (tfs_args_copy*) malloc(sizeof(tfs_args_copy));
    args_copy->path = path;
    args_copy->path2 = path2;

    tfs_args_write* args_write = (tfs_args_write*) malloc(sizeof(tfs_args_write));
    args_write->path = path;
    args_write->flag = TFS_O_APPEND;
    args_write->str = input1;
    args_write->to_write = sizeof(input1);
    
    if (pthread_create(&tid[i++], NULL, copy, (void *)args_copy) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, write_t, (void *)args_write) != 0) {
        exit(EXIT_FAILURE);
    }
    
    for (i = 0; i < 2; i++) {
        pthread_join(tid[i], (void **)&r[i]);
        printf("r[%d]: %d\n", i, *r[i]);
    }
    assert((*r[0] != -1) && (*r[1] != -1));

    printf("Successful test.\n");
    return 0;
}