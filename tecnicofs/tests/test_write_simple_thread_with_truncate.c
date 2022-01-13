#include "fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

typedef struct {
    char const *path;
    int flag;
    char *str;
    size_t to_write;
} tfs_args;

void *open1(void *args) {
    int fhandle;
    int *exit_val = (int *) malloc(sizeof(int));
    *exit_val = 0;
    tfs_args *_args = (tfs_args *) args;
    fhandle = tfs_open(_args->path, _args->flag);
    if (fhandle == -1) {
        *exit_val = -1;
        pthread_exit((void *) exit_val);
    }
    if (tfs_write(fhandle, _args->str, _args->to_write) != _args->to_write) {
        printf("Write failed\n");
        *exit_val = -1;
    }
    if (tfs_close(fhandle) == -1) {
        *exit_val = -1;
        pthread_exit((void *) exit_val);
    }
    pthread_exit((void *) exit_val);
}

int main() {

    pthread_t tid[3];

    int i = 0;
    int *r[3];
    char *path1 = "/f1";
    //char *path2 = "/f2";
    char buffer[40];
    char *str1 = "AAAAAAAAAA";
    char *str2 = "BBBBBBBBBB";
    char *str3 = "CCCCCCCCCC";
    //char str1[1536];
    //char str2[1536];
    //char str3[1536];
    //memset(str1, 'A', 1536);
    //memset(str2, 'B', 1536);
    //memset(str3, 'C', 1536);

    int fhandle;
    ssize_t size;

    assert(tfs_init() != -1);

    fhandle = tfs_open(path1, TFS_O_CREAT);
    assert(tfs_close(fhandle) != -1);


    tfs_args *_args_1 = (tfs_args*) malloc(sizeof(tfs_args));
    _args_1->path = path1;
    _args_1->flag = TFS_O_CREAT;
    _args_1->str = str1;
    _args_1->to_write = strlen(str1);
    tfs_args *_args_2 = (tfs_args*) malloc(sizeof(tfs_args));
    _args_2->path = path1;
    _args_2->flag = TFS_O_APPEND;
    _args_2->str = str2;
    _args_2->to_write = strlen(str2);
    tfs_args *_args_3 = (tfs_args*) malloc(sizeof(tfs_args));
    _args_3->path = path1;
    _args_3->flag = TFS_O_TRUNC;
    _args_3->str = str3;
    _args_3->to_write = strlen(str3);

    if (pthread_create(&tid[i++], NULL, open1, (void *)_args_1) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, open1, (void *)_args_2) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, open1, (void *)_args_3) != 0) {
        exit(EXIT_FAILURE);
    }
    
    for (i = 0; i < 3; i++) {
        pthread_join(tid[i], (void **)&r[i]);
        printf("r[%d]: %d\n", i, *r[i]);
        assert(*r[i] != -1);
    }

    fhandle = tfs_open(path1, 0);
    assert(fhandle != -1);

    size = tfs_read(fhandle, buffer, sizeof(buffer)-1);
    printf("size: %ld\n", size);
    //assert(size == 2*strlen(str1));

    buffer[size] = '\0';
    printf("Read: %s\n", buffer);
    //assert(strcmp(buffer, "CCCCCCCCCC") == 0);

    assert(tfs_close(fhandle) != -1);

    printf("Successful test.\n");

    return 0;
}