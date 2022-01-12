#include "fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>

typedef struct {
    char const *path;
    char *str;
    int exit_val;
} tfs_args;


void *test1(void *void_args) {
    tfs_args *args = (tfs_args *)void_args;

    int f;
    ssize_t r;
    char buffer[40];

    f = tfs_open(args->path, TFS_O_CREAT);
    assert(f != -1);

    r = tfs_write(f, args->str, strlen(args->str));
    assert(r == strlen(args->str));

    assert(tfs_close(f) != -1);

    f = tfs_open(args->path, 0);
    assert(f != -1);

    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(args->str));

    buffer[r] = '\0';
    assert(strcmp(buffer, args->str) == 0);

    assert(tfs_close(f) != -1);

    pthread_exit(NULL);
}

int main() {
    pthread_t tid[3];

    int i;
    char *str = "AAAAAAAAA!";
    char *str2 = "BBBBBBBBB!";
    char *path = "/f1";

    assert(tfs_init() != -1);

    tfs_args *args[2];
    for (i = 0; i < 2; i++) {
        args[i] = (tfs_args*) malloc(sizeof(tfs_args));
        args[i]->path = path;
    }
    args[0]->str = str;
    args[1]->str = str2;

    for (i = 0; i < 2; i++) {
        if (pthread_create (&tid[i], NULL, test1, (void*)args[i]) != 0) {
            free(args[0]);
            free(args[1]);
            exit(EXIT_FAILURE);
        }
    }

    for(i = 0; i < 2; i++) {
        pthread_join(tid[i], NULL);
    }

    tfs_destroy();

    free(args[0]);
    free(args[1]);

    printf("Successful test.\n");

    return 0;
}