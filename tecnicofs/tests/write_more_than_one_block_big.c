#include "fs/operations.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

#define COUNT 80
#define SIZE 5200

/**
   This test fills in a new file up to 20 blocks via multiple writes
   (therefore causing the file to hold 10 direct references + 10 indirect
   references from a reference block),
   each write always targeting only 1 block of the file, 
   then checks if the file contents are as expected
 */


int main() {

    char *path = "/f1";
    char *input = "Ok so I don't really know what to write here but I gotta get this to about maybe 3000 characters just to see if writing a big amount of characters in one go will work in tfs_write, I think that in the current way it is it will not but I'm not a hundred percent sure so that's why I'm writing this. Hopefully it isn't a big fix, I think that if I just sum what I just wrote to the buffer it should work but in case this test doesn't work I'll try out that theory, if that theory doesn't work, then we need to take some more drastic measures and search the webs, IKR very drastic. Alright this should be big enough... I hope. Ok... so correction this was only about 600 characters long so I'm gonna need to write a little more, to be honest I should just have copied so random thing from the internet or just pressed some random characters and then applied diff to know if it was the same but there was a slight chance this might have not worked because I could have randomly pressed the same sequence of characters at just the right time, which is something that surely will not happen if I do this, because I'm obviously not pressing the same exact sequence of characters twice so yeah... I'm pretty sure this is still not enough but I'm gonna try and see how many we already have. GGs good luck me.";
    char output[SIZE];
    printf("%ld\n", strlen(input));
    ssize_t r;

    assert(tfs_init() != -1);

    /* Write input COUNT times into a new file */
    int fd = tfs_open(path, TFS_O_CREAT);
    assert(fd != -1);
    for (int i = 0; i < 4; i++) {
        assert(tfs_write(fd, input, strlen(input)) == strlen(input));
    }
    
    assert(tfs_close(fd) != -1);

    /* Open again to check if contents are as expected */
    fd = tfs_open(path, 0);
    assert(fd != -1 );

    r = tfs_read(fd, output, sizeof(output)-1);
    output[r] = '\0';
    printf("r: %ld\n%s\n", r, output);

    /*
    for (int i = 0; i < COUNT; i++) {
        r = tfs_read(fd, output, 599);
        output[r] = '\0';
        printf("r: %ld\n%s\n", r, output);
        //assert(tfs_read(fd, output, SIZE) == SIZE);

        //assert (memcmp(input, output, SIZE) == 0);
    }
    */
    assert(tfs_close(fd) != -1);

    return 0;
}