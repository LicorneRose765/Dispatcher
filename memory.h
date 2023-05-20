/*
 * This file contains all the functions needed to manage the shared memory.
 * The shared memory consist of a memory handler which contains :
 *      - The maximum size of the memory (the number of blocks)
 *      - The current number of block claimed
 *      - A mutex to protect the claim of blocks
 *      - An array of pointers to blocks
 *
 * The blocks are composed of :
 *     - A block id (the index in the array)
 *     - A semaphore to wait for the block to be filled
 *     - Data to store
 *
 * Currently, the data stored in the blocks are the requests from the clients.
 * Data is composed of :
 *    - The type of the request
 *    - The delay to deal the request
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
block_t initBlock(unsigned int id) {
    block_t block;
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
        mem[i] = initBlock(i);
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
 * The client write a request to the dispatcher and push it to his block.
 * @param block The block of the client
 * @param data The data to send to the dispatcher
 */
void  clientWritingRequest(block_t *block, unsigned int request_size, request_t *data) {
    block-> data_size = request_size;
    for(int i=0; i< request_size; i++) {
        block->data[i] = data[i];
    }
}

/**
 * Gets the data stored on the block.
 * The dispatcher needs to call this function after he received a signal from the client/guichet.
 * @param block The block to get the data from
 * @return The data stored on the block (can be either a request or a response)
 */
request_t *dispatcherGetData(block_t *block) {
    return block->data;
}

/**
 * The dispatcher write a response to the client and push it to his block.
 * @param block The block of the client
 * @param data The data to send to the client
 */
void dispatcherWritingResponse(block_t *block, unsigned int response_size, request_t *data) {
    block->data_size = response_size;
    for (int i = 0; i < response_size; i++) {
        block->data[i] = data[i];
    }
    sem_post(&block->semaphore);
}

/**
 * The client wait for the dispatcher to write a response to his block.
 * @param block The block of the client
 * @return The list of responses
 */
request_t *clientWaitData(block_t *block) {
    sem_wait(&block->semaphore);
    return block->data;
}
