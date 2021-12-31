#include "fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

typedef struct {
    int fhandle;
    char *str;
    size_t to_read;
} tfs_args;

void *open1(void *args) {
    ssize_t read;
    int *exit_val = (int *) malloc(sizeof(int));
    *exit_val = 0;
    tfs_args *_args = (tfs_args *) args;
    read = tfs_read(_args->fhandle, _args->str, _args->to_read);
    if (read != _args->to_read) {
        printf("Read failed\n");
        *exit_val = -1;
    }
    _args->str[read] = '\0';
    printf("%s\n", _args->str);
    pthread_exit((void *) exit_val);
}

int main() {

    pthread_t tid[4];

    int i = 0;
    int *r[4];
    char *path1 = "/f1";
    //char *path2 = "/f2";
    char buffer[4][40];
    char *str1 = "AAAAAAAAAA";
    char *str2 = "BBBBBBBBBB";
    //char *str3 = "CCCCCCCCCC";

    int fhandle;
    ssize_t size;

    assert(tfs_init() != -1);

    fhandle = tfs_open(path1, TFS_O_CREAT);
    assert(fhandle != -1);

    size = tfs_write(fhandle, str1, strlen(str1));
    assert(size == strlen(str1));

    size = tfs_write(fhandle, str2, strlen(str2));
    assert(size == strlen(str2));

    assert(tfs_close(fhandle) != -1);

    fhandle = tfs_open(path1, TFS_O_CREAT);
    assert(fhandle != -1);

    tfs_args *_args_1 = (tfs_args*) malloc(sizeof(tfs_args));
    _args_1->fhandle = fhandle;
    _args_1->str = buffer[0];
    _args_1->to_read = 5;
    tfs_args *_args_2 = (tfs_args*) malloc(sizeof(tfs_args));
    _args_2->fhandle = fhandle;
    _args_2->str = buffer[1];
    _args_2->to_read = 5;
    tfs_args *_args_3 = (tfs_args*) malloc(sizeof(tfs_args));
    _args_3->fhandle = fhandle;
    _args_3->str = buffer[2];
    _args_3->to_read = 5;
    tfs_args *_args_4 = (tfs_args*) malloc(sizeof(tfs_args));
    _args_4->fhandle = fhandle;
    _args_4->str = buffer[3];
    _args_4->to_read = 5;


    if (pthread_create(&tid[i++], NULL, open1, (void *)_args_1) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, open1, (void *)_args_2) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, open1, (void *)_args_3) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, open1, (void *)_args_4) != 0) {
        exit(EXIT_FAILURE);
    }
    
    for (i = 0; i < 4; i++) {
        pthread_join(tid[i], (void **)&r[i]);
        printf("r[%d]: %d\n", i, *r[i]);
        assert(*r[i] != -1);
    }

    assert(tfs_close(fhandle) != -1);

    printf("Successful test.\n");

    return 0;
}