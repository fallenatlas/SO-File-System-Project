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
    ssize_t exit_val;
} tfs_args;

/* The objective of this test is to test multiple writes and reads in multiple files at the
   same time.
   This test is devided in 3 main phases of write, read and write and read respectively.
*/

void *write_read_big(void *void_args) {
    tfs_args *args = (tfs_args*) void_args;

    int i;
    int fd = tfs_open(args->path, args->flag);
    if (fd == -1) {
        printf("OPEN FAILED\n");
        args->exit_val = -1;
        pthread_exit(NULL);
    }
    char output[args->to_read+1];

    for(i = 0; i < args->count && args->exit_val != -1; i++) {
        if (tfs_write(fd, args->str, args->to_write) != args->to_write) {
            printf("WRITE FAILED\n");
            args->exit_val = -1;
        }
    }

    for(i = 0; i < args->count && args->exit_val != -1; i++) {
        args->exit_val = (int) tfs_read(fd, output, sizeof(output)-1);
        if (args->exit_val != sizeof(output)-1) {
            printf("READ FAILED\n");
            args->exit_val = -1;
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
        args->exit_val = -1;
    }

    pthread_exit(NULL);
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

void *write_big_thread(void *void_args) {
    tfs_args *args = (tfs_args *)void_args;

    if (write_big(void_args) == -1) {
        args->exit_val = -1;
    }

    pthread_exit(NULL);
}

void *read_big_thread(void *void_args) {
    tfs_args *args = (tfs_args *)void_args;

    args->exit_val = read_big(void_args);

    pthread_exit(NULL);
}

void *open_write_read_big(void *void_args) {
    tfs_args *args = (tfs_args *)void_args;

    if (write_big(void_args) == -1) {
        args->exit_val = -1;
    }

    if (read_big(void_args) == -1) {
        args->exit_val = -1;
    }

    pthread_exit(NULL);
}

void prepare_args(tfs_args *args, const char *path, int flag, int count, char *str, size_t to_read, size_t to_write) {
    args->path = path;
    args->flag = flag;
    args->count = count;
    args->str = str;
    args->to_read = to_read;
    args->to_write = to_write;
    args->exit_val = 0;
}


int main() {

    pthread_t tid[6];

    int i = 0;
    char *path1 = "/f1";
    char *path2 = "/f2";
    char *path3 = "/DFile";
    char str1[SIZE];
    char str2[SIZE];
    char str3[SIZE];
    char str4[SIZE];
    memset(str1, 'A', SIZE);
    memset(str2, 'B', SIZE);
    memset(str3, 'C', SIZE);
    memset(str4, 'D', SIZE);

    assert(tfs_init() != -1);

    tfs_args *_args_1 = (tfs_args*) malloc(sizeof(tfs_args));
    tfs_args *_args_2 = (tfs_args*) malloc(sizeof(tfs_args));
    tfs_args *_args_3 = (tfs_args*) malloc(sizeof(tfs_args));
    tfs_args *_args_4 = (tfs_args*) malloc(sizeof(tfs_args));
    tfs_args *_args_5 = (tfs_args*) malloc(sizeof(tfs_args));
    tfs_args *_args_6 = (tfs_args*) malloc(sizeof(tfs_args));

    /* Phase 1:
       Create 4 thread to write and then read the file from the beggining.
       3 threads will execute on one file and 1 will execute on another file.*/
    prepare_args(_args_1, path1, TFS_O_CREAT, COUNT_2, str1, SIZE, sizeof(str1));
    prepare_args(_args_2, path1, TFS_O_CREAT, COUNT_2, str1, SIZE, sizeof(str1));
    prepare_args(_args_3, path1, TFS_O_CREAT, COUNT_2, str1, SIZE, sizeof(str1));
    prepare_args(_args_4, path2, TFS_O_CREAT, COUNT_1, str3, SIZE, sizeof(str3));
    if (pthread_create(&tid[i++], NULL, open_write_read_big, (void *)_args_1) != 0) {
        free(_args_1);
        free(_args_2);
        free(_args_3);
        free(_args_4);
        free(_args_5);
        free(_args_6);
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, open_write_read_big, (void *)_args_2) != 0) {
        free(_args_1);
        free(_args_2);
        free(_args_3);
        free(_args_4);
        free(_args_5);
        free(_args_6);
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, open_write_read_big, (void *)_args_3) != 0) {
        free(_args_1);
        free(_args_2);
        free(_args_3);
        free(_args_4);
        free(_args_5);
        free(_args_6);
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, open_write_read_big, (void *)_args_4) != 0) {
        free(_args_1);
        free(_args_2);
        free(_args_3);
        free(_args_4);
        free(_args_5);
        free(_args_6);
        exit(EXIT_FAILURE);
    }
    
    for (i = 0; i < 4; i++) {
        pthread_join(tid[i], NULL);
    }

    assert(_args_1->exit_val != -1);
    assert(_args_2->exit_val != -1);
    assert(_args_3->exit_val != -1);
    assert(_args_4->exit_val != -1);
    printf("Phase 1 sucessful\n");


    /* Phase 2:
       2 threads will write to /f1 while another reads.
       1 thread will read from /f2 while another writes and then reads from the same file.
       1 thread will write and then read /DFile from the beggining.*/
    prepare_args(_args_1, path2, TFS_O_APPEND, COUNT_2, str1, SIZE, sizeof(str1));
    prepare_args(_args_2, path2, TFS_O_CREAT, COUNT_2, str2, SIZE, sizeof(str2));
    prepare_args(_args_3, path2, TFS_O_CREAT, COUNT_2, str3, SIZE, sizeof(str3));
    prepare_args(_args_4, path1, TFS_O_CREAT, COUNT_3, str2, SIZE, sizeof(str2));
    prepare_args(_args_5, path1, TFS_O_CREAT, COUNT_3, str2, SIZE, sizeof(str2));
    prepare_args(_args_6, path3, TFS_O_CREAT, COUNT_1, str4, SIZE, sizeof(str4));

    i = 0;
    if (pthread_create(&tid[i++], NULL, write_big_thread, (void *)_args_1) != 0) {
        free(_args_1);
        free(_args_2);
        free(_args_3);
        free(_args_4);
        free(_args_5);
        free(_args_6);
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, write_big_thread, (void *)_args_2) != 0) {
        free(_args_1);
        free(_args_2);
        free(_args_3);
        free(_args_4);
        free(_args_5);
        free(_args_6);
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, read_big_thread, (void *)_args_3) != 0) {
        free(_args_1);
        free(_args_2);
        free(_args_3);
        free(_args_4);
        free(_args_5);
        free(_args_6);
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, read_big_thread, (void *)_args_4) != 0) {
        free(_args_1);
        free(_args_2);
        free(_args_3);
        free(_args_4);
        free(_args_5);
        free(_args_6);
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, write_read_big, (void *)_args_5) != 0) {
        free(_args_1);
        free(_args_2);
        free(_args_3);
        free(_args_4);
        free(_args_5);
        free(_args_6);
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, open_write_read_big, (void *)_args_6) != 0) {
        free(_args_1);
        free(_args_2);
        free(_args_3);
        free(_args_4);
        free(_args_5);
        free(_args_6);
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < 6; i++) {
        pthread_join(tid[i], NULL);
    }

    assert(_args_1->exit_val != -1);
    assert(_args_2->exit_val != -1);
    assert(_args_3->exit_val != -1);
    assert(_args_4->exit_val != -1);
    assert(_args_5->exit_val != -1);
    assert(_args_6->exit_val != -1);
    printf("Phase 2 sucessful\n");


    /* Phase 3:
       Read the contents of the 3 files that were created and modified.*/
    prepare_args(_args_1, path1, TFS_O_CREAT, 1, "", COUNT_0, 0);
    prepare_args(_args_2, path2, TFS_O_CREAT, 1, "", COUNT_0, 0);
    prepare_args(_args_3, path2, TFS_O_CREAT, 1, "", COUNT_0, 0);
    prepare_args(_args_4, path3, TFS_O_CREAT, 1, "", COUNT_0, 0);

    i = 0;
    if (pthread_create(&tid[i++], NULL, read_big_thread, (void *)_args_1) != 0) {
        free(_args_1);
        free(_args_2);
        free(_args_3);
        free(_args_4);
        free(_args_5);
        free(_args_6);
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, read_big_thread, (void *)_args_2) != 0) {
        free(_args_1);
        free(_args_2);
        free(_args_3);
        free(_args_4);
        free(_args_5);
        free(_args_6);
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, read_big_thread, (void *)_args_3) != 0) {
        free(_args_1);
        free(_args_2);
        free(_args_3);
        free(_args_4);
        free(_args_5);
        free(_args_6);
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, read_big_thread, (void *)_args_4) != 0) {
        free(_args_1);
        free(_args_2);
        free(_args_3);
        free(_args_4);
        free(_args_5);
        free(_args_6);
        exit(EXIT_FAILURE);
    }

    int expected[4] = {6144, 21504, 21504, 15360};
    for(i = 0; i < 4; i++) {
        pthread_join(tid[i], NULL);
    }

    assert(_args_1->exit_val == expected[0]);
    assert(_args_2->exit_val == expected[1]);
    assert(_args_3->exit_val == expected[2]);
    assert(_args_4->exit_val == expected[3]);
    printf("Phase 3 sucessful\n");

    tfs_destroy();

    free(_args_1);
    free(_args_2);
    free(_args_3);
    free(_args_4);
    free(_args_5);
    free(_args_6);

    printf("Successful test.\n");

    return 0;
}