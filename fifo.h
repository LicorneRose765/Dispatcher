

typedef struct node {
    unsigned int serial_number;
    int task;
    int delay;
    struct node *next;
} node_t;

typedef struct{
    node_t *front;
    node_t *back;
    unsigned int size;
} Fifo;

/**
 * Initialize a new Queue
 * @return A pointer to the new Queue
 */
Fifo *createFifo() {
    Fifo *fifo = malloc(sizeof(Fifo));
    fifo->size = 0;
    fifo->front = NULL;
    fifo->back = NULL;
    return fifo;
}

/**
 * Create a node with the given parameters
 * @param serial_number The serial number of the request
 * @param task The task of the request
 * @param delay The delay to deal with the request
 * @return A pointer to the node
 */
node_t *createNode(unsigned int serial_number, int task, int delay) {
    node_t *node = (node_t *) malloc(sizeof(node_t));
    node->serial_number = serial_number;
    node->task = task;
    node->delay = delay;
    return node;
}

/**
 * Get the size of the Queue
 * @param fifo The Queue
 * @return The number of task currently in the Queue
 */
unsigned int size(Fifo *fifo) {
    return fifo->size;
}

/**
 * Check if the Queue is empty
 * @param fifo The Queue
 * @return 1 if the Queue is empty, 0 otherwise
 */
unsigned int isEmpty(Fifo *fifo) {
    return fifo->size == 0;
}

/**
 * Add a node to the Queue
 * @param fifo The Queue
 * @param node The node to add (the task)
 */
void enqueue(Fifo *fifo, node_t* node) {
    if (!isEmpty(fifo)) {
        fifo->back->next = node;
        fifo->back = node;
    } else {
        fifo->front = node;
        fifo->back = node;
    }
    fifo->size++;
}

/**
 * Remove the first node of the Queue
 * @param fifo The Queue
 * @return The node removed from the Queue, NULL if the Queue is empty
 */
node_t *dequeue(Fifo *fifo) {
    if (isEmpty(fifo)) {
        return NULL;
    }
    node_t *node = fifo->front;
    fifo->front = fifo->front->next;
    fifo->size--;
    return node;
}


void print_list(Fifo *fifo) {
    // TODO : remove this function
    node_t *current = fifo->front;
    while (current != NULL) {
        printf("%d ", current->serial_number);
        current = current->next;
    }
    printf("\n");
}
