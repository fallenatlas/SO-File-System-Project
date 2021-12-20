#include "operations.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int tfs_init() {
    state_init();

    /* create root inode */
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}

int tfs_destroy() {
    state_destroy();
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}


int tfs_lookup(char const *name) {
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    return find_in_dir(ROOT_DIR_INUM, name);
}

int tfs_open(char const *name, int flags) {
    int inum;
    size_t offset;

    /* Checks if the path name is valid */
    if (!valid_pathname(name)) {
        return -1;
    }

    inum = tfs_lookup(name);
    if (inum >= 0) {
        /* The file already exists */
        inode_t *inode = inode_get(inum);
        if (inode == NULL) {
            return -1;
        }

        /* Trucate (if requested) */
        if (flags & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                for(int i = 0; i < inode->n_data_blocks; i++) {
                    if (i < MAX_DIRECT_BLOCK) {
                        if (data_block_free(inode->i_data_block[i]) == -1) {
                            return -1;
                        }
                    }
                    else {
                        int *block_to_blocks = (int *) data_block_get(inode->i_data_block_to_data_blocks);
                        if (block_to_blocks == NULL)
                            return -1;
                        if (data_block_free(block_to_blocks[i-MAX_DIRECT_BLOCK]) == -1)
                            return -1;
                    }
                }
                if (inode->n_data_blocks > MAX_DIRECT_BLOCK) {
                    if (data_block_free(inode->i_data_block_to_data_blocks) == -1)
                        return -1;
                }
                inode->i_size = 0;
                inode->n_data_blocks = 0;
            }
        }
        /* Determine initial offset */
        if (flags & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
    } else if (flags & TFS_O_CREAT) {
        /* The file doesn't exist; the flags specify that it should be created*/
        /* Create inode */
        inum = inode_create(T_FILE);
        if (inum == -1) {
            return -1;
        }
        /* Add entry in the root directory */
        if (add_dir_entry(ROOT_DIR_INUM, inum, name + 1) == -1) {
            inode_delete(inum);
            return -1;
        }
        offset = 0;
    } else {
        return -1;
    }

    /* Finally, add entry to the open file table and
     * return the corresponding handle */
    return add_to_open_file_table(inum, offset);

    /* Note: for simplification, if file was created with TFS_O_CREAT and there
     * is an error adding an entry to the open file table, the file is not
     * opened but it remains created */
}


int tfs_close(int fhandle) { return remove_from_open_file_table(fhandle); }

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    open_file_entry_t *file = get_open_file_entry(fhandle);

    if (file == NULL) {
        return -1;
    }

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        return -1;
    }

    size_t written = 0;
    size_t to_write_now;
    size_t block_offset = file->of_offset;
    size_t n_block = 0;
    size_t current_block = block_offset / BLOCK_SIZE;
    size_t remain = block_offset % BLOCK_SIZE;
    int create_new_block = 0;

    block_offset = remain;
    if (remain == 0 && current_block > 0) {
        current_block--;
        block_offset = BLOCK_SIZE;
    }
    if (current_block > MAX_DIRECT_BLOCK)
        n_block = current_block - MAX_DIRECT_BLOCK;

    /*while(block_offset > BLOCK_SIZE) {
        block_offset -= BLOCK_SIZE;
        current_block++;
        if(current_block > MAX_DIRECT_BLOCK) { n_block++; }
    }*/

    while(to_write > 0) {
        if(block_offset == BLOCK_SIZE && current_block + 1 == inode->n_data_blocks) { create_new_block = 1; }

        if (to_write > BLOCK_SIZE && create_new_block == 1) {
            to_write_now = BLOCK_SIZE;
            to_write -= to_write_now;
        }
        else if(to_write + block_offset > BLOCK_SIZE && create_new_block == 0) {
            to_write_now = BLOCK_SIZE - block_offset;
            to_write -= to_write_now;
        }
        else {
            to_write_now = to_write;
            to_write = 0;
        }

        if (inode->i_size == 0) {
            /* If empty file, allocate new block */
            inode->i_data_block[0] = data_block_alloc();
        }
        else if(create_new_block == 1) {
            current_block++;
            block_offset = 0;
            if(current_block >= MAX_DIRECT_BLOCK) {
                if(current_block == MAX_DIRECT_BLOCK) {
                    inode->i_data_block_to_data_blocks = data_block_alloc();
                }
                else {
                    n_block++;
                    if (n_block > MAX_BLOCK_INDEX)
                        return (ssize_t)written;
                }
                int *block_to_blocks = (int *) data_block_get(inode->i_data_block_to_data_blocks);
                if (block_to_blocks == NULL)
                    return -1;
                block_to_blocks[n_block] = data_block_alloc();
            }
            else {
                inode->i_data_block[current_block] = data_block_alloc();
            }
        }

        void *block;
        if (current_block >= MAX_DIRECT_BLOCK) {
            int *block_to_blocks = (int *) data_block_get(inode->i_data_block_to_data_blocks);
            if (block_to_blocks == NULL)
                return -1;
            block = data_block_get(block_to_blocks[n_block]);
        }
        else {
            block = data_block_get(inode->i_data_block[current_block]);
        }

        if (block == NULL) {
            return -1;
        }

        if (create_new_block == 1 || inode->i_size == 0) {
            create_new_block = 0;
            inode->n_data_blocks++;
        }

        /* Perform the actual write */
        memcpy(block + block_offset, buffer, to_write_now);

        /* The offset associated with the file handle is
        * incremented accordingly */
        file->of_offset += to_write_now;
        block_offset += to_write_now;
        written += to_write_now;
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
    }
    return (ssize_t)written;
}


ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        return -1;
    }
    size_t block_offset = file->of_offset;
    size_t current_block = block_offset / BLOCK_SIZE;
    size_t remain = block_offset % BLOCK_SIZE;
    size_t n_block = 0;

    /* Determine how many bytes to read */
    size_t to_read = inode->i_size - file->of_offset;
    size_t to_read_now = 0;
    size_t read = 0;
    if (to_read > len) {
        to_read = len;
    }

    block_offset = remain;
    if (remain == 0 && current_block > 0) {
        current_block--;
        block_offset = BLOCK_SIZE;
    }
    if (current_block > MAX_DIRECT_BLOCK)
        n_block = current_block - MAX_DIRECT_BLOCK;

    /*while(block_offset > BLOCK_SIZE) {
        block_offset -= BLOCK_SIZE;
        current_block++;
        if(current_block > MAX_DIRECT_BLOCK) { n_block++; }
    }*/

    while (to_read > 0) {
        /* Reached end of a block, read next block */
        if(block_offset == BLOCK_SIZE) {
            current_block++;
            if(current_block > MAX_DIRECT_BLOCK) { n_block++; }
            block_offset = 0;
        }
        /* Need to read the rest of a block and more */
        if(to_read + block_offset > BLOCK_SIZE) {
            to_read_now = BLOCK_SIZE - block_offset;
            to_read -= to_read_now;
        }
        /* Can read all at once */
        else {
            to_read_now = to_read;
            to_read = 0;
        }

        /* Get block to read */
        void *block;
        if (current_block >= MAX_DIRECT_BLOCK) {
            /* Get indirect allocated block */
            int *block_to_blocks = (int *) data_block_get(inode->i_data_block_to_data_blocks);
            if (block_to_blocks == NULL)
                return -1;
            block = data_block_get(block_to_blocks[n_block]);
        }
        else {
            block = data_block_get(inode->i_data_block[current_block]);
        }

        if (block == NULL)
            return -1;

        /* Perform the actual read */
        memcpy(buffer, block + block_offset, to_read_now);
        /* The offset associated with the file handle is
         * incremented accordingly */
        file->of_offset += to_read_now;
        block_offset += to_read_now;
        read += to_read_now;
    }

    return (ssize_t)read;
}

int tfs_copy_to_external_fs(char const *source_path, char const *dest_path) {
    /* Open source file and read it */
    int fhandle = tfs_open(source_path, 0);
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        return -1;
    }
    size_t i_size = inode->i_size;
    char buffer[i_size];

    if (tfs_read(fhandle, buffer, i_size) == -1)
        return -1;

    /* Open destination file and copy source file */
    FILE *dest_file = fopen(dest_path, "w");
    if (dest_file == NULL)
        return -1;

    fwrite(buffer, sizeof(char), i_size, dest_file);
    if (ferror(dest_file) != 0)
        return -1;

    /* Close files */
    if (tfs_close(fhandle) == -1)
        return -1;
    if (fclose(dest_file) == EOF)
        return -1;

    return 0;
}
