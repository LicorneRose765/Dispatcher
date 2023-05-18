#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>


#define IPC_ERROR (-1)
#define WRITING_SUCCESS 1
#define WRITING_ERROR (-1)
#define BLOCK_SIZE 12

static int get_block(char *filename, int size) {
    key_t ipc_key;
    // Try to get a key for the file
    ipc_key = ftok(filename, 4242);
    // No key = error
    if (ipc_key == IPC_ERROR) return IPC_ERROR;
    // Use the key to create/get a block ID associated with the key
    return shmget(ipc_key, size, 0644 | IPC_CREAT);
}


char *load_block(char *filename, int size) {
    int block_id = get_block(filename, size);
    char *result;
    // No block ID = error
    if (block_id == IPC_ERROR) return NULL;
    // Map the block into the process' address space
    // to get a pointer to the block using the block ID
    result = shmat(block_id, NULL, 0);
    // No result = no block
    if (result == (char *) IPC_ERROR) return NULL;
    return result;
}


int detach_block(char *block) {
    return (shmdt(block) != IPC_ERROR);
}


int destroy_block(char *filename) {
    int id = get_block(filename, 0);
    if (id == IPC_ERROR) return -1;
    return (shmctl(id, IPC_RMID, NULL) != IPC_ERROR);
}


int write_to_block(char *str, char *block, char mode) {
    int remaining_space;
    switch (mode) {
        // Write mode (overwrite everything)
        case 'w':
            // Prevent buffer overflow
            if (strlen(str) > BLOCK_SIZE) {
                perror("Not enough space in block to write string.");
                return WRITING_ERROR;
            } else strncpy(block, str, BLOCK_SIZE);
            return WRITING_SUCCESS;
            // Append mode (add to end of existing data)
        case 'a':
            // Prevent buffer overflow
            remaining_space = BLOCK_SIZE - strlen(block);
            if (strlen(str) > remaining_space) {
                perror("Not enough space remaining in block to write string.");
                return WRITING_ERROR;
            } else strncpy(block, str, BLOCK_SIZE);
            return WRITING_SUCCESS;
        default:
            perror("Invalid mode.\n");
            return WRITING_ERROR;
    }
}