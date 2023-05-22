/*
 * This file contains all the functions needed to manage the shared memory between the clients and the dispatcher.
 * The shared memory consist of a memory handler which contains :
 *      - The maximum size of the memory (the number of blocks)
 *      - The current number of block claimed
 *      - A mutex to protect the claim of blocks
 *      - An array of pointers to blocks
 *
 * The blocks are composed of :
 *     - A block id (the index in the array)
 *     - A semaphore to wait for the block to be filled
 *     - A unsigned int to store the size of the data
 *     - Data to store (list of request)
 */

// TODO : comment this file correctly

typedef struct {
    unsigned int max_size;
    unsigned int current_size;
    pthread_mutex_t mutex;
    client_block_t *blocks;
} memory_handler_client_t;

memory_handler_client_t *client_memory_handler;

int shmid_client;

/**
 * Initialize a block with the given id
 * @param id The id of the block (the index in the array)
 * @return The block initialized
 */
client_block_t client_initBlock(unsigned int id) {
    client_block_t block;
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
int client_initMemoryHandler() {
    // Init the shared memory
    key_t ipc_key;
    // Try to get a key for the file
    ipc_key = ftok("application-shm", 4242);
    // No key = error
    if (ipc_key == IPC_ERROR) {
        perror("ftok client");
        return IPC_ERROR;
    }
    // Use the key to create/get a block ID associated with the key
    int size = sizeof(unsigned int) * 2 + sizeof(pthread_mutex_t) + sizeof(client_block_t) * (CLIENT_COUNT);
    printf("Size of the client shared memory : %d with key = %u\n", size, ipc_key);
    shmid_client = shmget(ipc_key, size, IPC_CREAT | 0600);
    if (shmid_client == IPC_ERROR) {
        perror("shmget client");
        return IPC_ERROR;
    }
    void *shmaddr = shmat(shmid_client, NULL, 0);
    if (shmaddr == (void *) IPC_ERROR) return IPC_ERROR;

    // The shared memory is a list of blocks
    client_memory_handler = (memory_handler_client_t *) shmaddr;
    client_memory_handler->max_size = CLIENT_COUNT;
    client_memory_handler->current_size = 0;
    pthread_mutex_init(&client_memory_handler->mutex, NULL);

    client_block_t *mem = (client_block_t *) (shmaddr + sizeof(unsigned int) * 2
            + sizeof(pthread_mutex_t)
            + sizeof(client_block_t *));
    // TODO : tbh idk why i have to add the sizeof(guichet_block_t *) but it works like that...
    for (int i=0; i<CLIENT_COUNT; i++) {
        mem[i] = client_initBlock(i);
    }

    client_memory_handler->blocks = mem;

    return EXIT_SUCCESS;
}

/**
 * Secure a block and gives the memory address of the block.
 * @return The memory address of the block, NULL if there is no block available.
 */
client_block_t *client_claimBlock(){
    pthread_mutex_lock(&client_memory_handler->mutex);
    if (client_memory_handler->current_size == client_memory_handler->max_size + 1) {
        pthread_mutex_unlock(&client_memory_handler->mutex);
        return NULL;
    }
    unsigned int id = client_memory_handler->current_size;
    client_memory_handler->current_size++;
    pthread_mutex_unlock(&client_memory_handler->mutex);
    return &client_memory_handler->blocks[id];
}

/**
 * Free the blocks and the memory handler.
 */
void client_destroyMemoryHandler() {
    for (int i=0; i < client_memory_handler->max_size; i++) {
        sem_destroy(&client_memory_handler->blocks[i].semaphore);
    }
    pthread_mutex_destroy(&client_memory_handler->mutex);

    if (shmctl(shmid_client, IPC_RMID, NULL) == -1) {
        perror("shmctl guicher");
    }

    shmdt(client_memory_handler);
}

/**
 * Get a block with the given id
 * @param id The id of the block
 * @return The block with the given id
 */
client_block_t *client_getBlock(unsigned int id) {
    return &client_memory_handler->blocks[id];
}

/**
 * The client write a request to the dispatcher and push it to his block.
 * @param block The block of the client
 * @param data The data to send to the dispatcher
 */
void  clientWritingRequest(client_block_t *block, unsigned int request_size, client_packet_t *data) {
    block-> data_size = request_size;
    for(int i=0; i< request_size; i++) {
        block->data[i] = data[i];
    }
    free(data);
}

/**
 * Gets the data stored on the block.
 * The dispatcher needs to call this function after he received a signal from the client.
 * @param block The block to get the data from
 * @return The data stored on the block (can be either a request or a response)
 */
client_packet_t *dispatcherGetDataFromClient(client_block_t *block) {
    return block->data;
}

/**
 * The dispatcher write a response to the client and push it to his block.
 * @param block The block of the client
 * @param data The data to send to the client
 */
void dispatcherWritingResponseForClient(client_block_t *block, unsigned int response_size, client_packet_t *data) {
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
client_packet_t *clientWaitData(client_block_t *block) {
    sem_wait(&block->semaphore);
    return block->data;
}
