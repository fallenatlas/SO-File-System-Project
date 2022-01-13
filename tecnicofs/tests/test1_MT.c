#include "fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>

typedef struct {
    char const *path;
    char *str;
    int exit_val;
} tfs_args;

/* Operations in different file handles but same file.
Two threads try to open the same file with TFS_O_CREATE flag (only one of them should create it
the other one should just open the created file), then try to write in the file overwriting its contents.
The files are opened again with offset 0 in order to read the file, since the contents were overwritten
the reading should be of str or str2, depending on the order of execution */
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
    /* Printf shows string that was written in this thread
    and what was read. They might be different from each other depending
    on the order of execution */
    printf("wrote: %s, read: %s\n", args->str, buffer);
    /* Could read what the other thread wrote? */
    assert(strcmp(buffer, args->str) == 0);

    assert(tfs_close(f) != -1);

    return NULL;
}

int main() {
    pthread_t tid[2];

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