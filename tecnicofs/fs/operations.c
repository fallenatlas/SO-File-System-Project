#include "operations.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

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
    if (state_destroy() != 0) {
        return -1;
    }
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

    lock_inode(ROOT_DIR_INUM, 'w');
    inum = tfs_lookup(name);
    if (inum >= 0) {
        unlock_inode(ROOT_DIR_INUM);
        /* The file already exists */
        inode_t *inode = inode_get(inum);
        if (inode == NULL) {
            return -1;
        }
        lock_inode(inum, 'w');
        /* Trucate (if requested) */
        if (flags & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                for(int i = 0; i < inode->n_data_blocks; i++) {
                    if (i < MAX_DIRECT_BLOCK) {
                        if (data_block_free(inode->i_data_block[i]) == -1) {
                            unlock_inode(inum);
                            return -1;
                        }
                    }
                    else {
                        int *block_to_blocks = (int *) data_block_get(inode->i_data_block_to_data_blocks);
                        if (block_to_blocks == NULL) {
                            unlock_inode(inum);
                            return -1;
                        }
                        if (data_block_free(block_to_blocks[i-MAX_DIRECT_BLOCK]) == -1) {
                            unlock_inode(inum);
                            return -1;
                        }
                    }
                }
                if (inode->n_data_blocks > MAX_DIRECT_BLOCK) {
                    if (data_block_free(inode->i_data_block_to_data_blocks) == -1) {
                        unlock_inode(inum);
                        return -1;
                    }
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
        unlock_inode(inum);
    } else if (flags & TFS_O_CREAT) {
        /* The file doesn't exist; the flags specify that it should be created*/
        /* Create inode */
        inum = inode_create(T_FILE);
        if (inum == -1) {
            unlock_inode(ROOT_DIR_INUM);
            return -1;
        }
        /* Add entry in the root directory */
        if (add_dir_entry(ROOT_DIR_INUM, inum, name + 1) == -1) {
            inode_delete(inum);
            unlock_inode(ROOT_DIR_INUM);
            return -1;
        }
        unlock_inode(ROOT_DIR_INUM);
        offset = 0;
    } else {
        unlock_inode(ROOT_DIR_INUM);
        return -1;
    }

    /* Finally, add entry to the open file table and
     * return the corresponding handle */
    return add_to_open_file_table(inum, offset, flags);

    /* Note: for simplification, if file was created with TFS_O_CREAT and there
     * is an error adding an entry to the open file table, the file is not
     * opened but it remains created */
}


int tfs_close(int fhandle) { return remove_from_open_file_table(fhandle); }

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    lock_open_file_entries(fhandle);
    open_file_entry_t *file = get_open_file_entry(fhandle);

    if (file == NULL) {
        unlock_open_file_entries(fhandle);
        return -1;
    }

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        unlock_open_file_entries(fhandle);
        return -1;
    }

    lock_inode(file->of_inumber, 'w');

    if (file->flag_mode & TFS_O_APPEND)
        /* File was oppened in TFS_O_APPEND mode change offset
        to the end of file before any writing */
        file->of_offset = inode->i_size;

    if (file->of_offset > inode->i_size){
        /* Offset is invalid */
        unlock_inode(file->of_inumber);
        unlock_open_file_entries(fhandle);
        return -1;
    }

    size_t written;
    size_t to_write_now;
    size_t block_offset = file->of_offset;
    /* Indirect block to write */
    size_t n_ind_block = 0;
    /* Block where offset is */
    size_t current_block = block_offset / BLOCK_SIZE;
    size_t remain = block_offset % BLOCK_SIZE;
    /* Flag that indicates if is necessary to allocate a new block:
    1 if needed and 0 if not */
    int create_new_block = 0;

    /* Determine how many bytes to write */
    if (to_write + file->of_offset > (BLOCK_SIZE * (MAX_DIRECT_BLOCK + MAX_BLOCK_INDEX))) {
        to_write = (BLOCK_SIZE * (MAX_DIRECT_BLOCK + MAX_BLOCK_INDEX)) - file->of_offset;
    }
    written = to_write;

    /* Determine the block where offset is */
    block_offset = remain;
    if (remain == 0 && current_block > 0) {
        /*Offset is in the end of the block */
        current_block--;
        block_offset = BLOCK_SIZE;
    }
    if (current_block > MAX_DIRECT_BLOCK)
        n_ind_block = current_block - MAX_DIRECT_BLOCK;

    while(to_write > 0) {
        /* Determine if is needed to allocate new block */
        if((block_offset == BLOCK_SIZE && current_block + 1 == inode->n_data_blocks) || (inode->i_size == 0)) {
            create_new_block = 1;
        }
        if (block_offset == BLOCK_SIZE) {
            block_offset = 0;
            /* Update block to write */
            current_block++;
            if (current_block > MAX_DIRECT_BLOCK)
                n_ind_block++;
        }

        /* Determine how many bytes can write in current block */
        to_write_now = BLOCK_SIZE - block_offset;
        if (to_write_now > to_write)
            to_write_now = to_write;
        to_write -= to_write_now;

        /* Allocate new block */
        if(create_new_block == 1) {
            if(current_block >= MAX_DIRECT_BLOCK) {
                if(current_block == MAX_DIRECT_BLOCK) {
                    inode->i_data_block_to_data_blocks = data_block_alloc();
                }
                int *block_to_blocks = (int *) data_block_get(inode->i_data_block_to_data_blocks);
                if (block_to_blocks == NULL) {
                    unlock_inode(file->of_inumber);
                    unlock_open_file_entries(fhandle);
                    return -1;
                }
                block_to_blocks[n_ind_block] = data_block_alloc();
            }
            else {
                inode->i_data_block[current_block] = data_block_alloc();
            }
        }

        /* Get block to write */
        void *block;
        if (current_block >= MAX_DIRECT_BLOCK) {
            int *block_to_blocks = (int *) data_block_get(inode->i_data_block_to_data_blocks);
            if (block_to_blocks == NULL) {
                unlock_inode(file->of_inumber);
                unlock_open_file_entries(fhandle);
                return -1;
            }
            block = data_block_get(block_to_blocks[n_ind_block]);
        }
        else {
            block = data_block_get(inode->i_data_block[current_block]);
        }

        if (block == NULL) {
            unlock_inode(file->of_inumber);
            unlock_open_file_entries(fhandle);
            return -1;
        }

        if (create_new_block == 1) {
            create_new_block = 0;
            inode->n_data_blocks++;
        }

        /* Perform the actual write */
        memcpy(block + block_offset, buffer, to_write_now);

        /* The offset associated with the file handle is
        * incremented accordingly */
        buffer += to_write_now;
        file->of_offset += to_write_now;
        block_offset += to_write_now;
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
    }
    unlock_inode(file->of_inumber);
    unlock_open_file_entries(fhandle);
    return (ssize_t)written;
}


ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    lock_open_file_entries(fhandle);
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        unlock_open_file_entries(fhandle);
        return -1;
    }

    /* From the open file table entry, we get the inode */
    inode_t *inode = inode_get(file->of_inumber);
    if (inode == NULL) {
        unlock_open_file_entries(fhandle);
        return -1;
    }
    lock_inode(file->of_inumber, 'r');
    /* If file was oppened in TFS_O_APPEND mode change offset to the end
    of file before any reading */
    if (file->flag_mode & TFS_O_APPEND)
        file->of_offset = inode->i_size;

    if (inode->i_size < file->of_offset) {
        /* Offset is invalid */
        unlock_inode(file->of_inumber);
        unlock_open_file_entries(fhandle);
        return -1;
    }
    size_t block_offset = file->of_offset;
    /* Block where offset is */
    size_t current_block = block_offset / BLOCK_SIZE;
    size_t remain = block_offset % BLOCK_SIZE;
    /* Indirect block to read */
    size_t n_ind_block = 0;

    /* Determine how many bytes to read */
    size_t to_read = inode->i_size - file->of_offset;
    size_t to_read_now;
    size_t read;
    if (to_read > len) {
        to_read = len;
    }
    read = to_read;

    /* Determine the block where offset is */
    block_offset = remain;
    if (remain == 0 && current_block > 0) {
        current_block--;
        block_offset = BLOCK_SIZE;
    }
    if (current_block > MAX_DIRECT_BLOCK)
        n_ind_block = current_block - MAX_DIRECT_BLOCK;

    while (to_read > 0) {
        /* Reached end of a block, read next block */
        if(block_offset == BLOCK_SIZE) {
            current_block++;
            if(current_block > MAX_DIRECT_BLOCK) { n_ind_block++; }
            block_offset = 0;
        }

        /* Determine how many bytes to read in current block */
        to_read_now = BLOCK_SIZE - block_offset;
        if (to_read_now > to_read)
            to_read_now = to_read;
        to_read -= to_read_now;

        /* Get block to read */
        void *block;
        if (current_block >= MAX_DIRECT_BLOCK) {
            /* Get indirect allocated block */
            int *block_to_blocks = (int *) data_block_get(inode->i_data_block_to_data_blocks);
            if (block_to_blocks == NULL) {
                unlock_inode(file->of_inumber);
                unlock_open_file_entries(fhandle);
                return -1;
            }
            block = data_block_get(block_to_blocks[n_ind_block]);
        }
        else {
            block = data_block_get(inode->i_data_block[current_block]);
        }

        if (block == NULL) {
            unlock_inode(file->of_inumber);
            unlock_open_file_entries(fhandle);
            return -1;
        }

        /* Perform the actual read */
        memcpy(buffer, block + block_offset, to_read_now);
        /* The offset associated with the file handle is
         * incremented accordingly */
        buffer += to_read_now;
        file->of_offset += to_read_now;
        block_offset += to_read_now;
    }
    unlock_inode(file->of_inumber);
    unlock_open_file_entries(fhandle);
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

    FILE *dest_file = fopen(dest_path, "w");
    if (dest_file == NULL)
        return -1;

    ssize_t read_now;
    size_t read = 0;
    char buffer[BLOCK_SIZE];

    while (inode->i_size > read) {
        read_now = tfs_read(fhandle, buffer, BLOCK_SIZE);
        if (read_now == -1) {
            return -1;
        }
        else {
            read += (size_t) read_now;
            fwrite(buffer, sizeof(char), (size_t) read_now, dest_file);
            if (ferror(dest_file) != 0)
                return -1;
        }
    }

    /* Close files */
    if (tfs_close(fhandle) == -1)
        return -1;
    if (fclose(dest_file) == EOF)
        return -1;

    return 0;
}
