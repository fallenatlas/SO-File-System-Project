#include "fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>

typedef struct {
    char const *path;
    int flag;
    int tid;
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
    int tid;
} tfs_read_args;

typedef struct {
    int fhandle;
    int tid;
} tfs_close_args;

int ret[2];
ssize_t wr[3];
int c[2];

void *tfs_open_thread(void *args) {
    tfs_open_args *open_args = (tfs_open_args *)args;
    ret[open_args->tid] = tfs_open(open_args->path, open_args->flag);
    return NULL;
}

void *tfs_write_thread(void *args) {
    tfs_write_args *write_args = (tfs_write_args *)args;
    wr[write_args->tid] = tfs_write(write_args->fhandle, write_args->str, write_args->to_write);
    return NULL;
}

void *tfs_read_thread(void *args) {
    tfs_read_args *read_args = (tfs_read_args *)args;
    wr[read_args->tid] = tfs_read(read_args->fhandle, read_args->str, read_args->to_read);
    return NULL;
}

void *tfs_close_thread(void *args) {
    tfs_close_args *close_args = (tfs_close_args *)args;
    c[close_args->tid] = tfs_close(close_args->fhandle);
    return NULL;
}

int main() {

    char *str = "AAA!";
    char *str2 = "BBB!";
    char *path = "/f1";
    char buffer[2][40];

    pthread_t tid[3];

    assert(tfs_init() != -1);

    int i;
    int f[2];
    ssize_t r;

    //Open the same file twice.
    tfs_open_args *open_args_1 = (tfs_open_args*) malloc(sizeof(tfs_open_args));
    open_args_1->path = path;
    open_args_1->flag = TFS_O_CREAT;
    open_args_1->tid = 0;
    if (pthread_create (&tid[0], NULL, tfs_open_thread, (void*)open_args_1) != 0) {
        free(open_args_1);
        exit(EXIT_FAILURE);
    }

    tfs_open_args *open_args_2 = (tfs_open_args*) malloc(sizeof(tfs_open_args));
    open_args_2->path = path;
    open_args_2->flag = TFS_O_CREAT;
    open_args_2->tid = 1;
    if (pthread_create (&tid[1], NULL, tfs_open_thread, (void*)open_args_2) != 0) {
        free(open_args_1);
        free(open_args_2);
        exit(EXIT_FAILURE);
    }

    //Check if opening occured correctly.
    for(i = 0; i < 2; i++) {
        pthread_join(tid[i], NULL);
        assert(ret[i] != -1);
        f[i] = ret[i];
    }

    //Create thread to write on the file through the first fhandle.
    tfs_write_args *write_args_1 = (tfs_write_args*) malloc(sizeof(tfs_write_args));
    write_args_1->fhandle = f[0];
    write_args_1->str = str;
    write_args_1->to_write = strlen(str);
    write_args_1->tid = 0;
    if (pthread_create (&tid[0], NULL, tfs_write_thread, (void*)write_args_1) != 0) {
            free(open_args_1);
            free(open_args_2);
            free(write_args_1);
            exit(EXIT_FAILURE);
    }

    //Create thread to write on the file through the second fhandle.
    tfs_write_args *write_args_2 = (tfs_write_args*) malloc(sizeof(tfs_write_args));
    write_args_2->fhandle = f[1];
    write_args_2->str = str;
    write_args_2->to_write = strlen(str);
    write_args_2->tid = 1;
    if (pthread_create (&tid[1], NULL, tfs_write_thread, (void*)write_args_2) != 0) {
            free(open_args_1);
            free(open_args_2);
            free(write_args_1);
            free(write_args_2);
            exit(EXIT_FAILURE);
    }

    //Check if both writes were succeful.
    for(i = 0; i < 2; i++) {
        pthread_join(tid[i], NULL);
        assert(wr[i] == strlen(str));
    }

    //Create threads to close both entries of the file.
    tfs_close_args *close_args_1 = (tfs_close_args*) malloc(sizeof(tfs_close_args));
    close_args_1->fhandle = f[0];
    close_args_1->tid = 0;
    if (pthread_create (&tid[0], NULL, tfs_close_thread, (void*)close_args_1) != 0) {
        free(open_args_1);
        free(open_args_2);
        free(write_args_1);
        free(write_args_2);
        free(close_args_1);
        exit(EXIT_FAILURE);
    }

    tfs_close_args *close_args_2 = (tfs_close_args*) malloc(sizeof(tfs_close_args));
    close_args_2->fhandle = f[1];
    close_args_2->tid = 1;
    if (pthread_create (&tid[1], NULL, tfs_close_thread, (void*)close_args_2) != 0) {
        free(open_args_1);
        free(open_args_2);
        free(write_args_1);
        free(write_args_2);
        free(close_args_1);
        free(close_args_2);
        exit(EXIT_FAILURE);
    }

    //Check if both closes were succeful.
    for(i = 0; i < 2; i++) {
        pthread_join(tid[i], NULL);
        assert(c[i] != -1);
    }

    free(write_args_1);
    free(write_args_2);

    //Open the same file twice
    if (pthread_create (&tid[0], NULL, tfs_open_thread, (void*)open_args_1) != 0) {
        free(open_args_1);
        exit(EXIT_FAILURE);
    }

    if (pthread_create (&tid[1], NULL, tfs_open_thread, (void*)open_args_2) != 0) {
        free(open_args_2);
        exit(EXIT_FAILURE);
    }

    //Check if opening occured correctly.
    for(i = 0; i < 2; i++) {
        pthread_join(tid[i], NULL);
        assert(ret[i] != -1);
        f[i] = ret[i];
    }

    //Read from both files openings
    tfs_read_args *read_args_1 = (tfs_read_args*) malloc(sizeof(tfs_read_args));
    read_args_1->fhandle = f[0];
    read_args_1->str = buffer[0];
    read_args_1->to_read = strlen(str);
    read_args_1->tid = 0;
    if (pthread_create (&tid[0], NULL, tfs_read_thread, (void*)read_args_1) != 0) {
        free(open_args_1);
        free(open_args_2);
        free(read_args_1);
        exit(EXIT_FAILURE);
    }

    tfs_read_args *read_args_2 = (tfs_read_args*) malloc(sizeof(tfs_read_args));
    read_args_2->fhandle = f[1];
    read_args_2->str = buffer[1];
    read_args_2->to_read = strlen(str);
    read_args_2->tid = 1;
    if (pthread_create (&tid[1], NULL, tfs_read_thread, (void*)read_args_2) != 0) {
        free(open_args_1);
        free(open_args_2);
        free(read_args_1);
        free(read_args_2);
        exit(EXIT_FAILURE);
    }

    //Write on the file through the first fhandle.
    tfs_write_args *write_args = (tfs_write_args*) malloc(sizeof(tfs_write_args));
    write_args->fhandle = f[0];
    write_args->str = str2;
    write_args->to_write = strlen(str);
    write_args->tid = 2;
    if (pthread_create (&tid[2], NULL, tfs_write_thread, (void*)write_args) != 0) {
            free(open_args_1);
            free(open_args_2);
            free(read_args_1);
            free(read_args_2);
            free(write_args);
            exit(EXIT_FAILURE);
    }

    //Check if both read and the write were succeful.
    int fail_count = 0;
    for(i = 0; i < 3; i++) {
        pthread_join(tid[i], NULL);
        assert(wr[i] == strlen(str) || ((i < 2) && (fail_count < 2)));
        if(wr[i] != strlen(str)) {
            fail_count += 1;
        }

        if(i < 2) {
            buffer[i][wr[i]] = '\0';
            assert(strcmp(buffer[i], str) == 0 || strcmp(buffer[i], "") == 0);
        }
    }

    //Create threads to close both entries of the file.
    if (pthread_create (&tid[0], NULL, tfs_close_thread, (void*)close_args_1) != 0) {
        exit(EXIT_FAILURE);
    }

    if (pthread_create (&tid[1], NULL, tfs_close_thread, (void*)close_args_2) != 0) {
        exit(EXIT_FAILURE);
    }

    //Check if both closes were succeful.
    for(i = 0; i < 2; i++) {
        pthread_join(tid[i], NULL);
        assert(c[i] != -1);
    }

    //Open file again to check if write was correct
    f[0] = tfs_open(path, TFS_O_CREAT);
    assert(f[0] != -1);

    //Read contents of the file
    r = tfs_read(f[0], buffer[0], sizeof(buffer) - 1);
    size_t possible_size_1 = 2*strlen(str);
    size_t possible_size_2 = 3*strlen(str);
    assert(r == possible_size_1 || r == possible_size_2);
    buffer[0][r] = '\0';

    //Check if its correct
    assert(strcmp(buffer[0], "BBB!AAA!") == 0 || strcmp(buffer[0], "AAA!BBB!") == 0 || strcmp(buffer[0], "AAA!AAA!BBB!"));

    assert(tfs_close(f[0]) != -1);

    printf("Successful test.\n");

    return 0;
}