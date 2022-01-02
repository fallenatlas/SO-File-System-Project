#include "fs/operations.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#define COUNT_0 25000
#define COUNT_1 10
#define COUNT_2 4
#define COUNT_3 2
#define SIZE 1536

/**
   This test fills in a new file up to 20 blocks via multiple writes
   (therefore causing the file to hold 10 direct references + 10 indirect
   references from a reference block),
   each write always targeting only 1 block of the file, 
   then checks if the file contents are as expected
 */

typedef struct {
    char const *path;
    int flag;
    int count;
    char *str;
    int to_read;
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
    char output[SIZE+1];

    for(i = 0; i < args->count && *exit_val != -1; i++) {
        if (tfs_write(fd, args->str, SIZE) != SIZE) {
            printf("WRITE FAILED\n");
            *exit_val = -1;
        }
    }

    for(i = 0; i < args->count && *exit_val != -1; i++) {
        if (tfs_read(fd, output, sizeof(output)-1) != SIZE) {
            printf("READ FAILED\n");
            *exit_val = -1;
        }
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

    //printf("%ld: %s\n", strlen(args->str), args->str+(SIZE-1));
    for(int i = 0; i < args->count && exit_val != -1; i++) {
        exit_val = (int) tfs_write(fd, args->str, SIZE);
        printf("    WRITE %s: %d %d\n", args->path, exit_val, i);
        if (exit_val != SIZE) {
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

int read_big(void *void_args) {
    tfs_args *args = (tfs_args*) void_args;

    int exit_val = 0;
    int fd = tfs_open(args->path, args->flag);
    if (fd == -1) {
        printf("OPEN FAILED\n");
        return -1;
    }
    char output[args->to_read+1];
    printf("    OPENED\n");

    for(int i = 0; i < args->count && exit_val != -1; i++) {
        exit_val = (int) tfs_read(fd, output, sizeof(output)-1);
        printf("    READ %s: %d, %d\n", args->path, exit_val, i);
        if (exit_val != args->to_read) {
            printf("READ FAILED\n");
            exit_val = -1;
        }
    }

    if (tfs_close(fd) == -1) {
        printf("CLOSE FAILED\n");
        exit_val = -1;
    }
    printf("    CLOSED\n");

    return exit_val;
}

void *read_all(void *void_args) {
    int *exit_val = (int *) malloc(sizeof(int));
    
    *exit_val = read_big(void_args);

    return (void *) exit_val;
}

void *write_big_thread(void *void_args) {
    int *exit_val = (int *) malloc(sizeof(int));
    *exit_val = 0;

    if (write_big(void_args) == -1) {
        *exit_val = -1;
    }

    pthread_exit((void *) exit_val);
}

void *read_big_thread(void *void_args) {
    int *exit_val = (int *) malloc(sizeof(int));
    *exit_val = 0;

    if (read_big(void_args) == -1) {
        *exit_val = -1;
    }

    pthread_exit((void *) exit_val);
}

void *open_write_read_big(void *void_args) {
    /*int *exit_val = (int *) malloc(sizeof(int));
    *exit_val = 0;

    printf("  WRITING\n");
    if (write_big(void_args) == -1) {
        *exit_val = -1;
    }

    printf("  READING\n");
    if (read_big(void_args) == -1) {
        *exit_val = -1;
    }
    printf("  FINISHED READING\n");

    pthread_exit((void *) exit_val);
    */
    
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
        if (tfs_write(fd, args->str, SIZE) != SIZE) {
            printf("WRITE FAILED\n");
            *exit_val = -1;
        }
    }

    if (tfs_close(fd) == -1) {
        printf("CLOSE FAILED\n");
        *exit_val = -1;
    }

    fd = tfs_open(args->path, args->flag);
    if (fd == -1) {
        printf("OPEN FAILED\n");
        *exit_val = -1;
        pthread_exit((void *) exit_val);
    }

    for(i = 0; i < args->count && *exit_val != -1; i++) {
        if (tfs_read(fd, output, sizeof(output)-1) != sizeof(output)-1) {
            printf("READ FAILED\n");
            *exit_val = -1;
        }
    }

    if (tfs_close(fd) == -1) {
        printf("CLOSE FAILED\n");
        *exit_val = -1;
    }

    pthread_exit((void *) exit_val);
}

void prepare_args(tfs_args *args, const char *path, int flag, int count, char *str, int to_read) {
    args->path = path;
    args->flag = flag;
    args->count = count;
    args->str = str;
    args->to_read = to_read;
}

int main() {

    pthread_t tid[6];

    char *path1 = "/f1";
    char *path2 = "/ficheiro2";
    //char *path3 = "/DFile";
    /*
    char *input = "Ok so I don't really know what to write here but I gotta get this to about maybe 3000 characters just to see if writing a big amount of characters in one go will work in tfs_write, I think that in the current way it is it will not but I'm not a hundred percent sure so that's why I'm writing this. Hopefully it isn't a big fix, I think that if I just sum what I just wrote to the buffer it should work but in case this test doesn't work I'll try out that theory, if that theory doesn't work, then we need to take some more drastic measures and search the webs, IKR very drastic. Alright this should be big enough... I hope. Ok... so correction this was only about 600 characters long so I'm gonna need to write a little more, to be honest I should just have copied so random thing from the internet or just pressed some random characters and then applied diff to know if it was the same but there was a slight chance this might have not worked because I could have randomly pressed the same sequence of characters at just the right time, which is something that surely will not happen if I do this, because I'm obviously not pressing the same exact sequence of characters twice so yeah... I'm pretty sure this is still not enough but I'm gonna try and see how many we already have. GGs good luck me.";
    char output[SIZE];
    */
    int i;
    ssize_t *r[6];
    char input1[SIZE];
    char input2[SIZE];
    char input3[SIZE];
    char input4[SIZE];
    memset(input1, 'A', SIZE);
    memset(input2, 'B', SIZE);
    memset(input3, 'C', SIZE);
    memset(input4, 'D', SIZE);
    //char output[4][SIZE+1];

    tfs_args *args_1 = (tfs_args*) malloc(sizeof(tfs_args));
    tfs_args *args_2 = (tfs_args*) malloc(sizeof(tfs_args));
    tfs_args *args_3 = (tfs_args*) malloc(sizeof(tfs_args));
    tfs_args *args_4 = (tfs_args*) malloc(sizeof(tfs_args));
    //tfs_args *args_5 = (tfs_args*) malloc(sizeof(tfs_args));
    //tfs_args *args_6 = (tfs_args*) malloc(sizeof(tfs_args));

    prepare_args(args_1, path1, TFS_O_CREAT, COUNT_2, input1, SIZE);
    prepare_args(args_2, path1, TFS_O_CREAT, COUNT_2, input1, SIZE);
    prepare_args(args_3, path1, TFS_O_CREAT, COUNT_2, input1, SIZE);
    prepare_args(args_4, path2, TFS_O_CREAT, COUNT_1, input3, SIZE);

    /*
    for(i = 0; i < 4; i++) {
        args[i]->flag = TFS_O_CREAT;
        if(i<3) {
            args[i]->path = path1;
            args[i]->count = COUNT_2;
            args[i]->str = input[0];
            args[i]->to_read = SIZE;
        } else {
            args[i]->path = path2;
            args[i]->count = COUNT_1;
            args[i]->str = input[2]; 
            args[i]->to_read = SIZE;
        }
    }
    */

    assert(tfs_init() != -1);

    printf("1st phase\n");

    i = 0;
    if (pthread_create(&tid[i++], NULL, open_write_read_big, (void *)args_1) != 0) {
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid[i++], NULL, open_write_read_big, (void *)args_2) != 0) {
        exit(EXIT_FAILURE);
    }
    /*if (pthread_create(&tid[i++], NULL, open_write_read_big, (void *)args_3) != 0) {
        exit(EXIT_FAILURE);
    }*/
    if (pthread_create(&tid[i++], NULL, open_write_read_big, (void *)args_4) != 0) {
        exit(EXIT_FAILURE);
    }


    /*
    for(i = 0; i < 4; i++) {
        if (pthread_create(&tid[i], NULL, open_write_read_big, (void *)args[i]) != 0) {
            exit(EXIT_FAILURE);
        }
    }
    */
    for(i = 0; i < 4; i++) {
        printf("WAITING: %d\n", i);
        pthread_join(tid[i], (void **)&r[i]);
        printf("Joined\n");
        assert(*r[i] != -1);
    }
    
    /*
    printf("SET: 2nd phase\n");
    for(i = 0; i < 6; i++) {
        if(i<3) {
            args[i]->path = path2;
            args[i]->count = COUNT_2;
            args[i]->str = input[i];
            args[i]->to_read = SIZE;
            if (i == 0) {
                args[i]->flag = TFS_O_APPEND;
            } else {
                args[i]->flag = TFS_O_CREAT;
            }
        } else if(3 < i && i < 5) {
            args[i]->path = path1;
            args[i]->count = COUNT_3;
            args[i]->str = input[1];
            args[i]->flag = TFS_O_CREAT;
            args[i]->to_read = SIZE;
        } else {
            args[i]->path = path3;
            args[i]->count = COUNT_1;
            args[i]->str = input[3]; 
            args[i]->flag = TFS_O_CREAT;
            args[i]->to_read = SIZE;
        }
    }

    printf("2nd phase\n");
    for(i = 0; i < 6; i++) {
        if (i < 2) {
            if (pthread_create(&tid[i], NULL, write_big_thread, (void *)args[i]) != 0) {
                exit(EXIT_FAILURE);
            }
        } else if (2 <= i && i <= 3) {
            if (pthread_create(&tid[i], NULL, read_big_thread, (void *)args[i]) != 0) {
                exit(EXIT_FAILURE);
            }
        } else if (i == 4) {
            if (pthread_create(&tid[i], NULL, write_read_big, (void *)args[i]) != 0) {
                exit(EXIT_FAILURE);
            }
        } else {
            if (pthread_create(&tid[i], NULL, open_write_read_big, (void *)args[i]) != 0) {
                exit(EXIT_FAILURE);
            }
        }
    }

    for(i = 0; i < 6; i++) {
        pthread_join(tid[i], (void **)&r[i]);
        assert(*r[i] != -1);
    }

    //read all from the 3 files
    for(i = 0; i < 4; i++) {
        args[i]->flag = TFS_O_CREAT;
        if(i<1) {
            args[i]->path = path1;
            args[i]->count = 1;
            args[i]->to_read = COUNT_0;
        } else if (1 < i  && i < 3) {
            args[i]->path = path2;
            args[i]->count = 1;
            args[i]->to_read = COUNT_0;
        } else {
            args[i]->path = path3;
            args[i]->count = 1;
            args[i]->to_read = COUNT_0;
        }
    }

    printf("3rd phase\n");
    for(i = 0; i < 4; i++) {
        if (pthread_create(&tid[i], NULL, read_big_thread, (void *)args[i]) != 0) {
            exit(EXIT_FAILURE);
        }
    }

    int expected[4] = {6144, 21504, 21504, 15360};
    for(i = 0; i < 4; i++) {
        pthread_join(tid[i], (void **)&r[i]);
        assert(*r[i] == expected[i]);
    }
    */
    return 0;
}