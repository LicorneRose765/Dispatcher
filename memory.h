/*
 * This file contains all the functions needed to manage the shared memory.
 * The shared memory consist of a memory handler which contains :
 *      - The maximum size of the memory (the number of blocks)
 *      - The current number of block claimed
 *      - A mutex to protect the claim of blocks
 *      - An array of pointers to blocks
 *
 * We store pointers to blocks in the share memory to reduce the size of the shared memory
 * (it still uses memory but not the shared one ;) ).
 */

typedef struct {
    unsigned int max_size;
    unsigned int current_size;
    pthread_mutex_t mutex;
    block_t *blocks;
} memory_handler_t;

memory_handler_t *memory_handler;

/**
 * Initialize a block with the given id
 * @param id The id of the block (the index in the array)
 * @return The block initialized
 */
block_t *initBlock(unsigned int id) {
    block_t *block = malloc(sizeof(block_t));
    block->block_id = id;
    if (sem_init(&block->semaphore, SEM_PUBLIC, 0) != 0) {
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
int initMemoryHandler() {
    // Init the shared memory
    key_t ipc_key;
    // Try to get a key for the file
    ipc_key = ftok("application-shm", 4242);
    // No key = error
    if (ipc_key == IPC_ERROR) {
        perror("ftok");
        return IPC_ERROR;
    }
    // Use the key to create/get a block ID associated with the key
    int size = sizeof(unsigned int) * 2 + sizeof(pthread_mutex_t) + sizeof(block_t *) * (CLIENT_COUNT+GUICHET_COUNT);
    printf("Size of the shared memory : %d with key = %u\n", size, ipc_key);
    int shmid = shmget(ipc_key, size, IPC_CREAT | 0600);
    if (shmid == IPC_ERROR) {
        perror("shmget");
        return IPC_ERROR;
    }
    void *shmaddr = shmat(shmid, NULL, 0);
    if (shmaddr == (void *) IPC_ERROR) return IPC_ERROR;

    // The shared memory is a list of blocks
    memory_handler = (memory_handler_t *) shmaddr;
    memory_handler->max_size = CLIENT_COUNT+GUICHET_COUNT;
    memory_handler->current_size = 0;
    pthread_mutex_init(&memory_handler->mutex, NULL);

    block_t *mem = (block_t *) (shmaddr + sizeof(unsigned int) * 2 + sizeof(pthread_mutex_t) + sizeof(block_t *));
    for (int i=0; i<CLIENT_COUNT+GUICHET_COUNT; i++) {
        mem[i] = *initBlock(i);
    }

    memory_handler->blocks = mem;

    return EXIT_SUCCESS;
}

/**
 * Secure a block and gives the memory address of the block.
 * @return The memory address of the block, NULL if there is no block available.
 */
block_t *claimBlock(){
    pthread_mutex_lock(&memory_handler->mutex);
    if (memory_handler->current_size == memory_handler->max_size+1) {
        pthread_mutex_unlock(&memory_handler->mutex);
        return NULL;
    }
    unsigned int id = memory_handler->current_size;
    memory_handler->current_size++;
    pthread_mutex_unlock(&memory_handler->mutex);
    return &memory_handler->blocks[id];
}

/**
 * Free the blocks and the memory handler.
 */
void destroyMemoryHandler() {
    for (int i=0; i<memory_handler->max_size; i++) {
        sem_destroy(&memory_handler->blocks[i].semaphore);
    }
    pthread_mutex_destroy(&memory_handler->mutex);
    shmdt(memory_handler);
    free(memory_handler);
}

/**
 * Get a block with the given id
 * @param id The id of the block
 * @return The block with the given id
 */
block_t *getBlock(unsigned int id) {
    return &memory_handler->blocks[id];
}


/**
 * The client wait for a response from the dispatcher. After the dispatcher pushed a response,
 * the client can get the response.
 * @param block The block of the client
 * @return The response of the dispatcher
 */
request_group_t *clientWaitingResponse(block_t *block) {
    sem_wait(&block->semaphore);
    return block->data;
}

/**
 * The client write a request to the dispatcher and push it to his block.
 * @param block The block of the client
 * @param data The data to send to the dispatcher
 */
void clientWritingRequest(block_t *block, request_group_t *data) {
    printf("Client %d writing request with %d task\n", block->block_id, data->num_requests);
    printf("[TEST] first task : %d with delay %ld\n", data->requests[0].type, data->requests[0].delay);
    block->data = data;
}

/**
 * Gets the data stored on the block.
 * The dispatcher needs to call this function after he received a signal from the client/guichet.
 * @param block The block to get the data from
 * @return The data stored on the block (can be either a request or a response)
 */
request_group_t *dispatcherGetData(block_t *block) {
    return block->data;
}


/**
 * The dispatcher push a response to the client block and post the semaphore to notify the client.
 * @param block The block of the client
 * @param data The data to send to the client
 * @return
 */
void dispatcherWritingResponse(block_t *block, request_group_t *data) {
    block->data = data;
    sem_post(&block->semaphore);
}

/**
 * The guichet waits for the dispatcher to push a request to his block.
 * @param block The block of the guichet
 * @return The request sent by the dispatcher
 */
request_group_t *guichetWaitingRequest(block_t *block) {
    sem_wait(&block->semaphore);
    return block->data;
}

/**
 * The guichet writes the response to
 * @param block
 * @param data
 */
void guichetWritingResponse(block_t *block, request_group_t *data) {
    block->data = data;
}