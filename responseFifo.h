/*
 * This fifo works the same way as fifo.h but the nodes store a packet of request instead of a single packet.
 * We use this to store the packets to send when the dispatcher will restart the work at 6am.
 */

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

/**
 * Creates a response node to store in the fifo
 * @param packet The array of packet to send to the client
 * @param packetSize The number of packet in the array
 * @param clientID The id of the client
 * @return The response node created with the given parameters
 */
response_node_t *createResponseNode(client_packet_t *packet, unsigned int packetSize, unsigned int clientID) {
    response_node_t *node = malloc(sizeof(response_node_t));
    node->packetSize = packetSize;
    node->packet = packet;
    node->clientID = clientID;
}

/**
 * check if the Queue is empty
 * @param fifo The response Queue
 * @return 1 if the Queue is empty, 0 otherwise
 */
unsigned int isResponseFifoEmpty(response_fifo *fifo) {
    return fifo->size == 0;
}

/**
 * Add a response_node at the end of the queue
 * @param fifo The response queue
 * @param node The node to add
 */
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

/**
 * Remove the first node of the Queue
 * @param fifo The response Queue
 * @return The node removed from the Queue, NULL if the Queue is empty
 */
response_node_t *dequeueResponse(response_fifo *fifo) {
    if (isResponseFifoEmpty(fifo)) {
        return NULL;
    }
    response_node_t *node = fifo->front;
    fifo->front = fifo->front->next;
    fifo->size--;
    return node;
}