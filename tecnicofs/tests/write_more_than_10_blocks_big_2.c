#include "fs/operations.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define COUNT_0 25000
#define COUNT_1 10
#define COUNT_2 4
#define COUNT_3 2
#define SIZE 1536

typedef struct {
    char const *path;
    int flag;
    int count;
    char *str;
    size_t to_read;
    size_t to_write;
} tfs_args;

void *write_read_big(void *void_args) {
    tfs_args *args = (tfs_args*) void_args;

    int *exit_val = (int *) malloc(sizeof(int));
    *exit_val = 0;

    int i;
    int fd = tfs_open(args->path, args->flag);
    if (fd == -1) {
        printf("OPEN FAILED\n");
        *exit_val = -1;
        pthread_exit((void *) exit_val);
    }
    char output[args->to_read+1];

    for(i = 0; i < args->count && *exit_val != -1; i++) {
        if (tfs_write(fd, args->str, args->to_write) != args->to_write) {
            printf("WRITE FAILED\n");
            *exit_val = -1;
        }
    }

    for(i = 0; i < args->count && *exit_val != -1; i++) {
        *exit_val = (int) tfs_read(fd, output, sizeof(output)-1);
        if (*exit_val != sizeof(output)-1) {
            printf("READ FAILED\n");
            *exit_val = -1;
        }
        /*
        else {
            output[*exit_val] = '\0';
            printf("read: %s\n", output);
        }
        */
    }

    if (tfs_close(fd) == -1) {
        printf("CLOSE FAILED\n");
        *exit_val = -1;
    }

    pthread_exit((void *) exit_val);
}

int write_big(void *void_args) {
    tfs_args *args = (tfs_args*) void_args;

    int exit_val = 0;
    int fd = tfs_open(args->path, args->flag);
    if (fd == -1) {
        printf("OPEN FAILED\n");
        return -1;
    }

    for(int i = 0; i < args->count && exit_val != -1; i++) {
        if (tfs_write(fd, args->str, args->to_write) != args->to_write) {
            printf("WRITE FAILED\n");
            exit_val = -1;
        }
    }

    if (tfs_close(fd) == -1) {
        printf("CLOSE FAILED\n");
        exit_val = -1;
    }

    return exit_val;
}

ssize_t read_big(void *void_args) {
    tfs_args *args = (tfs_args*) void_args;

    ssize_t exit_val = 0;
    int fd = tfs_open(args->path, args->flag);
    if (fd == -1) {
        printf("OPEN FAILED\n");
        return -1;
    }
    char output[args->to_read+1];

    for(int i = 0; i < args->count && exit_val != -1; i++) {
        exit_val = tfs_read(fd, output, sizeof(output)-1);
        if (exit_val != args->to_read && args->to_read != COUNT_0) {
            printf("READ FAILED\n");
            exit_val = -1;
        }
        /*
        else {
            output[exit_val] = '\0';
            printf("read: %s\n", output);
        }
        */
    }

    if (tfs_close(fd) == -1) {
        printf("CLOSE FAILED\n");
        exit_val = -1;
    }

    return exit_val;
}

/*
void *read_all(void *void_args) {
    ssize_t *exit_val = (ssize_t *) malloc(sizeof(int));
    
    *exit_val = read_big(void_args);

    return (void *) exit_val;
}
*/

void *write_big_thread(void *void_args) {
    int *exit_val = (int *) malloc(sizeof(int));
    *exit_val = 0;

    if (write_big(void_args) == -1) {
        *exit_val = -1;
    }

    pthread_exit((void *) exit_val);
}

void *read_big_thread(void *void_args) {
    ssize_t *exit_val = (ssize_t *) malloc(sizeof(ssize_t));
    *exit_val = 0;

    *exit_val = read_big(void_args);

    pthread_exit((void *) exit_val);
}

void *open_write_read_big(void *void_args) {
    int *exit_val = (int *) malloc(sizeof(int));
    *exit_val = 0;

    if (write_big(void_args) == -1) {
        *exit_val = -1;
    }

    if (read_big(void_args) == -1) {
        *exit_val = -1;
    }

    pthread_exit((void *) exit_val);
}

/*
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
    printf("to_write: %ld\n", _args->to_write);
    for (int i = 0; i < _args->count; i++) {
        printf("going to write: %c\n", _args->str[0]);
        if (tfs_write(fhandle, _args->str, _args->to_write) != _args->to_write) {
            printf("Write failed\n");
            *exit_val = -1;
        }   
    }
    if (tfs_close(fhandle) == -1) {
        *exit_val = -1;
        pthread_exit((void *) exit_val);
    }
    pthread_exit((void *) exit_val);
}
*/

void prepare_args(tfs_args *args, const char *path, int flag, int count, char *str, size_t to_read, size_t to_write) {
    args->path = path;
    args->flag = flag;
    args->count = count;
    args->str = str;
    args->to_read = to_read;
    args->to_write = to_write;
}


int main() {

    pthread_t tid[6];

    int i = 0;
    int *r[6];
    char *path1 = "/f1";
    char *path2 = "/f2";
    char *path3 = "/DFile";
    //char buffer[COUNT_0];
    char str1[SIZE];
    char str2[SIZE];
    char str3[SIZE];
    char str4[SIZE];
    memset(str1, 'A', SIZE);
    memset(str2, 'B', SIZE);
    memset(str3, 'C', SIZE);
    memset(str4, 'D', SIZE);

    //int fhandle;
    //ssize_t size;

    assert(tfs_init() != -1);

    tfs_args *_args_1 = (tfs_args*) malloc(sizeof(tfs_args));
    tfs_args *_args_2 = (tfs_args*) malloc(sizeof(tfs_args));
    tfs_args *_args_3 = (tfs_args*) malloc(sizeof(tfs_args));
    tfs_args *_args_4 = (tfs_args*) malloc(sizeof(tfs_args));
    tfs_args *_args_5 = (tfs_args*) malloc(sizeof(tfs_args));
    tfs_args *_args_6 = (tfs_args*) malloc(sizeof(tfs_args));

    // Phase 1:
    // Create 4 thread to write and then read the file from the beggining.
    // 3 threads will execute on one file and 1 will execute on another file.
    prepare_args(_args_1, path1, TFS_O_CREAT, COUNT_2, str1, SIZE, sizeof(str1));
    prepare_args(_args_2, path1, TFS_O_CREAT, COUNT_2, str1, SIZE, sizeof(str1));
    prepare_args(_args_3, path1, TFS_O_CREAT, COUNT_2, str1, SIZE, sizeof(str1));
    prepare_args(_args_4, path2, TFS_O_CREAT, COUNT_1, str3, SIZE, sizeof(str3));
    if (pthread_create(&tid[i++], NULL, open_write_read_big, (void *)_args_1) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, open_write_read_big, (void *)_args_2) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, open_write_read_big, (void *)_args_3) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, open_write_read_big, (void *)_args_4) != 0) {
        exit(EXIT_FAILURE);
    }
    
    for (i = 0; i < 4; i++) {
        pthread_join(tid[i], (void **)&r[i]);
        printf("r[%d]: %d\n", i, *r[i]);
        assert(*r[i] != -1);
    }


    // Phase 2:
    // 2 threads will write to /f1 while another reads.
    // 1 thread will read from /f2 while another writes and then reads from the same file.
    // 1 thread will write and then read /DFile from the beggining.
    prepare_args(_args_1, path2, TFS_O_APPEND, COUNT_2, str1, SIZE, sizeof(str1));
    prepare_args(_args_2, path2, TFS_O_CREAT, COUNT_2, str2, SIZE, sizeof(str2));
    prepare_args(_args_3, path2, TFS_O_CREAT, COUNT_2, str3, SIZE, sizeof(str3));
    prepare_args(_args_4, path1, TFS_O_CREAT, COUNT_3, str2, SIZE, sizeof(str2));
    prepare_args(_args_5, path1, TFS_O_CREAT, COUNT_3, str2, SIZE, sizeof(str2));
    prepare_args(_args_6, path3, TFS_O_CREAT, COUNT_1, str4, SIZE, sizeof(str4));

    i = 0;
    if (pthread_create(&tid[i++], NULL, write_big_thread, (void *)_args_1) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, write_big_thread, (void *)_args_2) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, read_big_thread, (void *)_args_3) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, read_big_thread, (void *)_args_4) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, write_read_big, (void *)_args_5) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, open_write_read_big, (void *)_args_6) != 0) {
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < 6; i++) {
        pthread_join(tid[i], (void **)&r[i]);
        printf("r[%d]: %d\n", i, *r[i]);
        assert(*r[i] != -1);
    }


    // Phase 3:
    // Read the contents of the 3 files that were created and modified.
    prepare_args(_args_1, path1, TFS_O_CREAT, 1, "", COUNT_0, 0);
    prepare_args(_args_2, path2, TFS_O_CREAT, 1, "", COUNT_0, 0);
    prepare_args(_args_3, path2, TFS_O_CREAT, 1, "", COUNT_0, 0);
    prepare_args(_args_4, path3, TFS_O_CREAT, 1, "", COUNT_0, 0);

    i = 0;
    if (pthread_create(&tid[i++], NULL, read_big_thread, (void *)_args_1) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, read_big_thread, (void *)_args_2) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, read_big_thread, (void *)_args_3) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, read_big_thread, (void *)_args_4) != 0) {
        exit(EXIT_FAILURE);
    }

    int expected[4] = {6144, 21504, 21504, 15360};
    for(i = 0; i < 4; i++) {
        pthread_join(tid[i], (void **)&r[i]);
        printf("r[%d]: %d\n", i, *r[i]);
        assert(*r[i] == expected[i]);
    }

    /*
    fhandle = tfs_open(path1, 0);
    assert(fhandle != -1);

    size = tfs_read(fhandle, buffer, sizeof(buffer)-1);
    assert(size == sizeof(str1)*4);

    buffer[size] = '\0';
    printf("Read: %s\n", buffer);
    //assert(strcmp(buffer, "AAAAAAAAAA") == 0 || strcmp(buffer, "BBBBBBBBBB") == 0 || strcmp(buffer, "CCCCCCCCCC") == 0);

    assert(tfs_close(fhandle) != -1);
    */
    printf("Successful test.\n");

    return 0;
}