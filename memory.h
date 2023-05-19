

typedef struct {
    block_t *blocks;
    unsigned int max_size;
    unsigned int current_size;

    pthread_mutex_t *mutex;
} memory_handler_t;

memory_handler_t *memory_handler;

block_t *initBlock(unsigned int id) {
    block_t *block = malloc(sizeof(block_t));
    block->block_id = id;
    sem_init(block->semaphore, 0, 0);
    return block;
}

int initMemoryHandler() {
    // Init the shared memory
    key_t ipc_key;
    // Try to get a key for the file
    ipc_key = ftok("application-shm", 4242);
    // No key = error
    if (ipc_key == IPC_ERROR) return IPC_ERROR;
    // Use the key to create/get a block ID associated with the key

    int size = CLIENT_COUNT* sizeof(block_t) + GUICHET_COUNT * sizeof(block_t);
    int shmid = shmget(ipc_key, size, 0644 | IPC_CREAT);
    if (shmid == IPC_ERROR) return IPC_ERROR;

    void *shmaddr = shmat(shmid, NULL, 0);
    if (shmaddr == (void *) IPC_ERROR) return IPC_ERROR;

    block_t *mem = (block_t *) shmaddr;
    for (int i=0; i<CLIENT_COUNT+GUICHET_COUNT; i++) {
        mem[i] = *initBlock(i);
        // TODO : verifier si c'est bien le bon truc (avec *)
    }

    memory_handler = malloc(sizeof(memory_handler_t));
    memory_handler->blocks = mem;
    memory_handler->max_size = CLIENT_COUNT+GUICHET_COUNT;
    memory_handler->current_size = 0;
    pthread_mutex_init(memory_handler->mutex, NULL);

    return EXIT_SUCCESS;
}

/**
 * Secure a block and gives the memory address of the block.
 * @return The memory address of the block, NULL if there is no block available.
 */
block_t *claimBlock(){
    pthread_mutex_lock(memory_handler->mutex);
    if (memory_handler->current_size == memory_handler->max_size) {
        pthread_mutex_unlock(memory_handler->mutex);
        return NULL;
    }
    unsigned int id = memory_handler->current_size;
    memory_handler->current_size++;
    pthread_mutex_unlock(memory_handler->mutex);
    return &memory_handler->blocks[id];
}

/**
 * Free the blocks and the memory handler.
 */
void destroyMemoryHandler() {
    free(memory_handler->blocks);
    pthread_mutex_destroy(memory_handler->mutex);
    shmdt(memory_handler);
    free(memory_handler);
}


void *clientWaitingResponse(block_t *block) {
    sem_wait(block->semaphore);
    return block->data;
}

void *dispatcherGettingClientRequest(block_t *block) {
    return block->data;
}

unsigned int dispatcherWritingResponse(block_t *block, void *data) {
    block->data = data;
    sem_post(block->semaphore);
    return 0;
}
