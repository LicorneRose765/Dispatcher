/*
 * This file contains all the functions needed to manage the shared memory for the desk.
 * The shared memory consist of a memory handler which contains :
 *     - The maximum size of the memory (the number of blocks)
 *     - The current number of block claimed
 *     - A mutex to protect the claim of blocks
 *     - An array of pointers to blocks
 *
 * The blocks for the desks are composed of :
 *    - A block id (the index in the array)
 *    - A semaphore to wait for the block to be filled
 *    - a desk_packet_t structure to store the work.
 */


typedef struct {
    unsigned int max_size;
    unsigned int current_size;
    pthread_mutex_t mutex;
    desk_block_t *blocks;
} memory__handler_desk_t;

memory__handler_desk_t *desk_memory_handler;

int shmid;


/**
 * Initialize a block with the given id
 * @param id The id of the block (the index in the array)
 * @return The block initialized
 */
desk_block_t desk_initBlock(unsigned int id) {
    desk_block_t block;
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
int desk_initMemoryHandler() {
    // Init the shared memory
    key_t ipc_key;
    // Try to get a key for the file
    ipc_key = ftok("desk-shm", 4242);
    // No key = error
    if (ipc_key == IPC_ERROR) {
        perror("ftok desk");
        return IPC_ERROR;
    }
    // Use the key to create/get a block ID associated with the key
    int size = sizeof(unsigned int) * 2 + sizeof(pthread_mutex_t) + sizeof(desk_block_t) * (DESK_COUNT);
    printf("Size of the desk shared memory : %d with key = %u\n", size, ipc_key);
    shmid = shmget(ipc_key, size, IPC_CREAT | 0600);
    if (shmid == IPC_ERROR) {
        perror("shmget desk");
        return IPC_ERROR;
    }
    // printf("shmid value = %d\n", shmid);
    void *shmaddr = shmat(shmid, NULL, 0);
    if (shmaddr == (void *) IPC_ERROR) return IPC_ERROR;

    // The shared memory is a list of blocks
    desk_memory_handler = (memory__handler_desk_t *) shmaddr;
    desk_memory_handler->max_size = DESK_COUNT;
    desk_memory_handler->current_size = 0;
    pthread_mutex_init(&desk_memory_handler->mutex, NULL);

    desk_block_t *mem = (desk_block_t *) (shmaddr + sizeof(unsigned int) * 2
                                          + sizeof(pthread_mutex_t)
                                          + sizeof(desk_block_t *));
    for (int i = 0; i < DESK_COUNT; i++) {
        mem[i] = desk_initBlock(i);
    }

    desk_memory_handler->blocks = mem;

    return EXIT_SUCCESS;
}

/**
 * Claim a block in the shared memory
 * @return The block claimed
 */
desk_block_t *desk_claimBlock() {
    pthread_mutex_lock(&desk_memory_handler->mutex);
    if (desk_memory_handler->current_size >= desk_memory_handler->max_size) {
        pthread_mutex_unlock(&desk_memory_handler->mutex);
        return NULL;
    }
    desk_block_t *block = &desk_memory_handler->blocks[desk_memory_handler->current_size];
    desk_memory_handler->current_size++;
    pthread_mutex_unlock(&desk_memory_handler->mutex);
    return block;
}

/**
* Free the blocks and the shared memory
*/
void desk_destroyMemoryHandler() {
    for (int i = 0; i < DESK_COUNT; i++) {
        sem_destroy(&desk_memory_handler->blocks[i].semaphore);
    }
    pthread_mutex_destroy(&desk_memory_handler->mutex);

    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl desk");
    }

    shmdt(desk_memory_handler);
}

/**
 * Get a block with the given id
 * @param id The id of the block
 * @return The block with the given id
 */
desk_block_t *desk_getBlock(unsigned int id) {
    return &desk_memory_handler->blocks[id];
}

/**
 * The desk waits for a work to be done
 * @param block The block of the desk
 * @return The work to do
 */
desk_packet_t desk_waitForWork(desk_block_t *block) {
    sem_wait(&block->semaphore);
    return block->data;
}

/**
 * The desk sends a work to the dispatcher
 * @param block The block of the desk
 * @param work The work to do
 */
void desk_dispatcherSendsWork(desk_block_t *block, desk_packet_t work) {
    block->data = work;
    sem_post(&block->semaphore);
}

/**
 * The desk sends the response to the dispatcher
 * @param block The block of the desk
 * @param response The response to send
 */
void desk_sendResponse(desk_block_t *block, desk_packet_t response) {
    block->data = response;
}

desk_packet_t dispatcherGetResponseFromDesk(desk_block_t *block) {
    return block->data;
}