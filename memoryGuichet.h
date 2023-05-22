/*
 * This file contains all the functions needed to manage the shared memory for the guichet.
 * The shared memory consist of a memory handler which contains :
 *     - The maximum size of the memory (the number of blocks)
 *     - The current number of block claimed
 *     - A mutex to protect the claim of blocks
 *     - An array of pointers to blocks
 *
 * The blocks for the guichets are composed of :
 *    - A block id (the index in the array)
 *    - A semaphore to wait for the block to be filled
 *    - A work structure to store the work to do.
 */

// TODO : comment this file correctly

typedef struct {
    unsigned int max_size;
    unsigned int current_size;
    pthread_mutex_t mutex;
    guichet_block_t *blocks;
} memory__handler_guichet_t;

memory__handler_guichet_t *guichet_memory_handler;

int shmid;


/**
 * Initialize a block with the given id
 * @param id The id of the block (the index in the array)
 * @return The block initialized
 */
guichet_block_t guichet_initBlock(unsigned int id) {
    guichet_block_t block;
    block.block_id = id;
    if (sem_init(&block.semaphore, SEM_PUBLIC, 0) != 0) {
        perror("Error while initializing semaphore");
        exit(EXIT_FAILURE);
    }
    return block;
}

/**
 * Initialize the memory share memory, all the blocks needed and prepare the handler
 * to claim blocks
 * @return 0 if success, -1 if error
 */
int guichet_initMemoryHandler() {
    // Init the shared memory
    key_t ipc_key;
    // Try to get a key for the file
    ipc_key = ftok("guichet-shm", 4242);
    // No key = error
    if (ipc_key == IPC_ERROR) {
        perror("ftok guichet");
        return IPC_ERROR;
    }
    // Use the key to create/get a block ID associated with the key
    int size = sizeof(unsigned int) * 2 + sizeof(pthread_mutex_t) + sizeof(guichet_block_t) * (GUICHET_COUNT);
    printf("Size of the guichet shared memory : %d with key = %u\n", size, ipc_key);
    shmid = shmget(ipc_key, size, IPC_CREAT | 0600);
    if (shmid == IPC_ERROR) {
        perror("shmget guichet");
        return IPC_ERROR;
    }
    // printf("shmid value = %d\n", shmid);
    void *shmaddr = shmat(shmid, NULL, 0);
    if (shmaddr == (void *) IPC_ERROR) return IPC_ERROR;

    // The shared memory is a list of blocks
    guichet_memory_handler = (memory__handler_guichet_t *) shmaddr;
    guichet_memory_handler->max_size = GUICHET_COUNT;
    guichet_memory_handler->current_size = 0;
    pthread_mutex_init(&guichet_memory_handler->mutex, NULL);

    guichet_block_t *mem = (guichet_block_t *) (shmaddr + sizeof(unsigned int) * 2
            + sizeof(pthread_mutex_t)
            + sizeof(guichet_block_t *));
    // TODO : tbh idk why i have to add the sizeof(guichet_block_t *) but it works like that...
    for (int i=0; i<GUICHET_COUNT; i++) {
        mem[i] = guichet_initBlock(i);
    }

    guichet_memory_handler->blocks = mem;

    return EXIT_SUCCESS;
}

/**
 * Claim a block in the shared memory
 * @return The block claimed
 */
guichet_block_t *guichet_claimBlock() {
    pthread_mutex_lock(&guichet_memory_handler->mutex);
    if (guichet_memory_handler->current_size >= guichet_memory_handler->max_size) {
        pthread_mutex_unlock(&guichet_memory_handler->mutex);
        return NULL;
    }
    guichet_block_t *block = &guichet_memory_handler->blocks[guichet_memory_handler->current_size];
    guichet_memory_handler->current_size++;
    pthread_mutex_unlock(&guichet_memory_handler->mutex);
    return block;
}

/**
* Free the blocks and the shared memory
*/
void guichet_destroyMemoryHandler() {
    for (int i=0; i < GUICHET_COUNT; i++) {
        sem_destroy(&guichet_memory_handler->blocks[i].semaphore);
    }
    pthread_mutex_destroy(&guichet_memory_handler->mutex);

    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl guicher");
    }

    shmdt(guichet_memory_handler);
};

/**
 * Get a block with the given id
 * @param id The id of the block
 * @return The block with the given id
 */
guichet_block_t *guichet_getBlock(unsigned int id) {
    return &guichet_memory_handler->blocks[id];
}

/**
 * The guichet waits for a work to be done
 * @param block The block of the guichet
 * @return The work to do
 */
guichet_packet_t guichet_waitForWork(guichet_block_t *block) {
    sem_wait(&block->semaphore);
    return block->data;
}

/**
 * The guichet sends a work to the dispatcher
 * @param block The block of the guichet
 * @param work The work to do
 */
void guichet_dispatcherSendsWork(guichet_block_t *block, guichet_packet_t work) {
    block->data = work;
    sem_post(&block->semaphore);
}

/**
 * The guichet sends the response to the dispatcher
 * @param block The block of the guichet
 * @param response The response to send
 */
void guichet_sendResponse(guichet_block_t *block, guichet_packet_t response) {
    block->data = response;
}

guichet_packet_t guichet_dispatcherGetResponseFromGuichet(guichet_block_t *block) {
    return block->data;
}