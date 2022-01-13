#include "fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>

typedef struct {
    char const *path;
    int flag;
    int tid;
    int exit_val;
} tfs_open_args;

typedef struct {
    int fhandle;
    char *str;
    size_t to_write;
    int tid;
    int exit_val;
} tfs_write_args;

typedef struct {
    int fhandle;
    char *str;
    size_t to_read;
    int tid;
    int exit_val;
} tfs_read_args;

typedef struct {
    int fhandle;
    int tid;
    int exit_val;
} tfs_close_args;

int ret[2];
ssize_t wr[3];
int c[2];

void *tfs_open_thread(void *args) {
    tfs_open_args *open_args = (tfs_open_args *)args;
    open_args->exit_val = tfs_open(open_args->path, open_args->flag);
    return NULL;
}

void *tfs_write_thread(void *args) {
    tfs_write_args *write_args = (tfs_write_args *)args;
    write_args->exit_val = tfs_write(write_args->fhandle, write_args->str, write_args->to_write);
    return NULL;
}

void *tfs_read_thread(void *args) {
    tfs_read_args *read_args = (tfs_read_args *)args;
    read_args->exit_val = tfs_read(read_args->fhandle, read_args->str, read_args->to_read);
    return NULL;
}

void *tfs_close_thread(void *args) {
    tfs_close_args *close_args = (tfs_close_args *)args;
    close_args->exit_val = tfs_close(close_args->fhandle);
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
    tfs_open_args *open_args[2];
    for (i = 0; i < 2; i++) {
        open_args[i] = (tfs_open_args*) malloc(sizeof(tfs_open_args));
        open_args[i]->path = path;
        open_args[i]->flag = TFS_O_CREAT;
    }
    for (i = 0; i < 2; i++) {
        if (pthread_create (&tid[i], NULL, tfs_open_thread, (void*)open_args[i]) != 0) {
            free(open_args[0]);
            free(open_args[1]);
            exit(EXIT_FAILURE);
        }
    }

    //Check if opening occured correctly.
    for(i = 0; i < 2; i++) {
        pthread_join(tid[i], NULL);
        assert(open_args[i]->exit_val != -1);
        f[i] = open_args[i]->exit_val;
    }

    //Create thread to write on the file through the first fhandle.
    tfs_write_args *write_args[2];
    for (i = 0; i < 2; i++) {
        write_args[i] = (tfs_write_args*) malloc(sizeof(tfs_write_args));
        write_args[i]->fhandle = f[i];
        write_args[i]->str = str;
        write_args[i]->to_write = strlen(str);
    }

    for (i = 0; i < 2; i++) {
        if (pthread_create (&tid[i], NULL, tfs_write_thread, (void*)write_args[i]) != 0) {
            free(open_args[0]);
            free(open_args[1]);
            free(write_args[0]);
            free(write_args[1]);
            exit(EXIT_FAILURE);
        }
    }

    //Check if both writes were succeful.
    for(i = 0; i < 2; i++) {
        pthread_join(tid[i], NULL);
        assert(write_args[i]->exit_val == strlen(str));
    }

    tfs_close_args *close_args[i];
    for (i = 0; i < 2; i++) {
        close_args[i] = (tfs_close_args*) malloc(sizeof(tfs_close_args));
        close_args[i]->fhandle = f[i];
    }
    //Create threads to close both entries of the file.
    for (i = 0; i < 2; i++) {
        if (pthread_create (&tid[i], NULL, tfs_close_thread, (void*)close_args[i]) != 0) {
            free(open_args[0]);
            free(open_args[1]);
            free(write_args[0]);
            free(write_args[1]);
            free(close_args[0]);
            free(close_args[1]);
            exit(EXIT_FAILURE);
        }
    }

    //Check if both closes were succeful.
    for(i = 0; i < 2; i++) {
        pthread_join(tid[i], NULL);
        assert(c[i] != -1);
    }

    for (i = 0; i < 2; i++) {
        free(write_args[i]);
    }

    //Open the same file twice
    for (i = 0; i < 2; i++) {
        if (pthread_create (&tid[i], NULL, tfs_open_thread, (void*)open_args[i]) != 0) {
            free(open_args[0]);
            free(open_args[1]);
            free(close_args[0]);
            free(close_args[1]);
            exit(EXIT_FAILURE);
        }
    }

    //Check if opening occured correctly.
    for(i = 0; i < 2; i++) {
        pthread_join(tid[i], NULL);
        assert(open_args[i]->exit_val != -1);
        f[i] = open_args[i]->exit_val;
    }

    tfs_read_args *read_args[2];
    tfs_write_args *write_args_2 = (tfs_write_args*) malloc(sizeof(tfs_write_args));
    for(i = 0; i < 2; i++) {
        read_args[i] = (tfs_read_args*) malloc(sizeof(tfs_read_args));
        read_args[i]->fhandle = f[i];
        read_args[i]->str = buffer[i];
        read_args[i]->to_read = strlen(str);
        if (i == 0) {
            write_args_2->fhandle = f[0];
            write_args_2->str = str2;
            write_args_2->to_write = strlen(str);
        }
    }
    //Read from both files openings
    for (i = 0; i < 2; i++) {
        if (pthread_create (&tid[i], NULL, tfs_read_thread, (void*)read_args[i]) != 0) {
            free(open_args[0]);
            free(open_args[1]);
            free(close_args[0]);
            free(close_args[1]);
            free(read_args[0]);
            free(read_args[1]);
            free(write_args_2);
            exit(EXIT_FAILURE);
        }
    }

    //Write on the file through the first fhandle.
    if (pthread_create (&tid[2], NULL, tfs_write_thread, (void*)write_args_2) != 0) {
            free(open_args[0]);
            free(open_args[1]);
            free(close_args[0]);
            free(close_args[1]);
            free(read_args[0]);
            free(read_args[1]);
            free(write_args_2);
            exit(EXIT_FAILURE);
    }


    //falta a partir daqui




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
            assert(strcmp(buffer[i], str) == 0 || strcmp(buffer[i], "") == 0 || strcmp(buffer[i], str2) == 0);
        }
    }

    //Create threads to close both entries of the file.
    for (i = 0; i < 2; i++) {
        if (pthread_create (&tid[i], NULL, tfs_close_thread, (void*)close_args[i]) != 0) {
            free(open_args[0]);
            free(open_args[1]);
            free(write_args[0]);
            free(write_args[1]);
            free(close_args[0]);
            free(close_args[1]);
            exit(EXIT_FAILURE);
        }
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
    size_t possible_size_1 = strlen(str);
    size_t possible_size_2 = 2*strlen(str);
    assert(r == possible_size_1 || r == possible_size_2);
    buffer[0][r] = '\0';

    //Check if its correct
    assert(strcmp(buffer[0], "BBB!") == 0 || strcmp(buffer[0], "AAA!BBB!")== 0);
    assert(tfs_close(f[0]) != -1);

    printf("Successful test.\n");

    return 0;
}