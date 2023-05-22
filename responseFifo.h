// TODO : comment the code here

typedef struct client_node {
    client_packet_t *packet;
    unsigned int packetSize;
    unsigned int clientID;
    struct client_node *next;
} response_node_t;

typedef struct {
    response_node_t *front;
    response_node_t *back;
    unsigned int size;
} response_fifo;

/**
 * Initialize a new queue for the client packets (Used to store packet during the night)
 * @return A point to the new Queue
 */
response_fifo *createResponseFifo() {
    response_fifo *fifo = malloc(sizeof(response_fifo));
    fifo->size = 0;
    fifo->front = NULL;
    fifo->back = NULL;
    return fifo;
}


response_node_t *createResponseNode(client_packet_t *packet, unsigned int packetSize, unsigned int clientID) {
    response_node_t *node = malloc(sizeof(response_node_t));
    node->packetSize = packetSize;
    node->packet = packet;
    node->clientID = clientID;
}

unsigned int isResponseFifoEmpty(response_fifo *fifo){
    return fifo->size == 0;
}

void enqueueResponse(response_fifo *fifo, response_node_t *node) {
    if (!isResponseFifoEmpty(fifo)) {
        fifo->back->next = node;
        fifo->back = node;
    } else {
        fifo->front = node;
        fifo->back = node;
    }
    fifo->size++;
}

response_node_t *dequeueResponse(response_fifo *fifo) {
    if (isResponseFifoEmpty(fifo)) {
        return NULL;
    }
    response_node_t *node = fifo->front;
    fifo->front = fifo->front->next;
    fifo->size--;
    return node;
}