#include "fs/operations.h"
#include <assert.h>
#include <string.h>

typedef struct {
    char const *path;
    int flag;
} tfs_open_args;

typedef struct {
    int fhandle;
    char *str;
    int to_write;
} tfs_write_args;

typedef struct {
    int fhandle;
    char *str;
    int to_read;
} tfs_read_args;

int main() {

    char *str = "AAA!";
    char *str2 = "BBB!";
    char *path = "/f1";
    char buffer[2][40];

    pthread_t tid[3];

    assert(tfs_init() != -1);

    int *f[2];
    ssize_t *r;
    int *c;

    //Open the same file twice.
    tfs_open_args *open_args = (tfs_open_args*) malloc(sizeof(tfs_open_args));
    open_args->path = path;
    open_args->flag = TFS_O_CREAT;
    for(int i = 0; i < 2; i++) {
        if (pthread_create (&tid[i], NULL, tfs_open, (void*)open_args) != 0) {
            free(open_args);
            exit(EXIT_FAILURE);
        }
    }

    //Check if opening occured correctly.
    for(int i = 0; i < 2; i++) {
        pthread_join(tid[i], &f[i]);
        assert(*f[i] != -1);
    }

    //Create thread to write on the file through the first fhandle.
    tfs_write_args *write_args_1 = (tfs_write_args*) malloc(sizeof(tfs_write_args));
    write_args_1->fhandle = f[0];
    write_args_1->str = str;
    write_args_1->to_write = strlen(str);
    if (pthread_create (&tid[0], NULL, tfs_write, (void*)write_args_1) != 0) {
            free(open_args);
            free(write_args_1);
            exit(EXIT_FAILURE);
    }

    //Create thread to write on the file through the second fhandle.
    tfs_write_args *write_args_2 = (tfs_write_args*) malloc(sizeof(tfs_write_args));
    write_args_2->fhandle = f[1];
    write_args_2->str = str;
    write_args_2->to_write = strlen(str);
    if (pthread_create (&tid[1], NULL, tfs_write, (void*)write_args_2) != 0) {
            free(open_args);
            free(write_args_1);
            free(write_args_2);
            exit(EXIT_FAILURE);
    }

    //Check if both writes were succeful.
    for(int i = 0; i < 2; i++) {
        pthread_join(&tid[i], &r);
        assert(*r == strlen(str));
    }

    //Create threads to close both entries of the file.
    for(int i = 0; i < 2; i++) {
        if (pthread_create (&tid[i], NULL, tfs_close, (void*)&f[i]) != 0) {
            free(open_args);
            free(write_args_1);
            free(write_args_2);
            exit(EXIT_FAILURE);
    }

    //Check if both closes were succeful.
    for(int i = 0; i < 2; i++) {
        pthread_join(&tid[i], &c);
        assert(*c != -1);
    }

    free(write_args_1);
    free(write_args_2);

    //Open the same file twice
    for(int i = 0; i < 2; i++) {
        if (pthread_create (&tid[i], NULL, tfs_open, (void*)open_args) != 0) {
            free(open_args);
            exit(EXIT_FAILURE);
        }
    }

    //Check if opening occured correctly.
    for(int i = 0; i < 2; i++) {
        pthread_join(tid[i], &f[i]);
        assert(*f[i] != -1);
    }

    //Read from both files openings
    tfs_read_args *read_args_1 = (tfs_read_args*) malloc(sizeof(tfs_read_args));
    read_args_1->fhandle = f[0];
    read_args_1->str = buffer[0];
    read_args_1->to_read = strlen(str);
    if (pthread_create (&tid[0], NULL, tfs_read, (void*)read_args_1) != 0) {
        free(open_args);
        free(read_args_1);
        exit(EXIT_FAILURE);
    }

    tfs_read_args *read_args_2 = (tfs_read_args*) malloc(sizeof(tfs_read_args));
    read_args_2->fhandle = f[1];
    read_args_2->str = buffer[1];
    read_args_2->to_read = strlen(str);
    if (pthread_create (&tid[1], NULL, tfs_read, (void*)read_args_2) != 0) {
        free(open_args);
        free(read_args_1);
        free(read_args_2);
        exit(EXIT_FAILURE);
    }

    //Write on the file through the first fhandle.
    tfs_write_args *write_args = (tfs_write_args*) malloc(sizeof(tfs_write_args));
    write_args->fhandle = f[0];
    write_args->str = str2;
    write_args->to_write = strlen(str);
    if (pthread_create (&tid[2], NULL, tfs_write, (void*)write_args) != 0) {
            free(open_args);
            free(read_args_1);
            free(read_args_2);
            free(write_args);
            exit(EXIT_FAILURE);
    }

    //Check if both read and the write were succeful.
    for(int i = 0; i < 3; i++) {
        pthread_join(&tid[i], &r);
        assert(*r == strlen(str));

        if(i < 2) {
            buffer[i][*r] = '\0';
            assert(strcmp(buffer[i], str) == 0);
        }
    }

    //Create threads to close both entries of the file.
    for(int i = 0; i < 2; i++) {
        if (pthread_create (&tid[i], NULL, tfs_close, (void*)&f[i]) != 0) {
            free(open_args);
            free(write_args_1);
            free(write_args_2);
            exit(EXIT_FAILURE);
        }
    }

    //Check if both closes were succeful.
    for(int i = 0; i < 2; i++) {
        pthread_join(&tid[i], &c);
        assert(*c != -1);
    }

    //Open file again to check if write was correct
    *f[0] = tfs_open(path, TFS_O_CREAT);
    assert(*f[0] != -1);

    //Read contents of the file
    r = tfs_read(f, buffer[0], sizeof(buffer) - 1);
    assert(*r == strlen(str));
    buffer[0][*r] = '\0';

    //Check if its correct
    assert(strcmp(buffer, str) == 0 || strcmp(buffer, "AAA!BBB!") == 0);

    assert(tfs_close(f[0]) != -1);

    printf("Successful test.\n");

    return 0;
}