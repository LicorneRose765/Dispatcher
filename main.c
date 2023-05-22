#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>


#include "utils.h"
#include "fifo.h"
#include "responseFifo.h"
#include "memoryClient.h"
#include "memoryDesk.h"
#include "client.h"
#include "desk.h"

/******************************************************************************
* S-INFO-111 --- Solution pour Travail 2                                      *
* Groupe 03                                                                   *
* Membres:                                                                    *
* - BERNARD Thomas                                                            *
* - MOREAU Arnaud                                                             *
* - MOREAU Cyril                                                              *
******************************************************************************/

/*
 * To solve this problem, we implemented two zones of shared memory.
 *      - Between the clients and the dispatcher
 *      - Between the desks (Guichets) and the dispatcher
 *
 * These zones are decomposed in blocks for each client/desk.
 * When one entity is created, it claims a block on the correct shared memory. The index of the block gives the ID of the entity
 *
 * See utils.h to change the parameters of the application.
 *
 * (We tried to comment the code as much as possible to make it very clear)
 * (There are some print line commented, if you want to see the execution of the program, you can uncomment them)
 */

sigset_t mask;
struct sigaction descriptor;
union sigval value;

long dispatcherTime = STARTING_TIME; // Virtual time
int dispatcherState; // State of the dispatcher (opened or closed

pid_t clientPID;
pid_t deskPID;

Fifo *desksQueues[DESK_COUNT]; // List of queues, one queue per desk, used to hold requests when the desk is busy
int desksOccupancy[DESK_COUNT]; // List of occupancies of desks


// ========= Dispatcher buffers & function for buffers ==========
// Keep track of the state of the desks
Fifo *dispatcherClientQueue; // Queue of requests received when the dispatcher is sleeping
response_fifo *fifoForSleepyDispatcher; // Queue of responses received when the dispatcher is sleeping

client_response_buffer clientResponseBuffer[CLIENT_COUNT]; // Buffer to store the response already received for each client

/**
 * Creates a buffer to wait for all the responses before sending the whole packet to the client
 * @param numberOfResponse The number of responses to wait
 * @param clientID The id of the client.
 */
void createNodeOnClientBuffer(unsigned int numberOfResponse, unsigned int clientID) {
    client_response_buffer buffer = {
            .response_to_wait = numberOfResponse,
            .responses = malloc(sizeof(client_packet_t) * numberOfResponse)
    };
    clientResponseBuffer[clientID] = buffer;
}


/**
 * Send all the responses to the client and free the buffer
 * @param clientID The id of the client
 */
void sendToClient(unsigned int clientID) {
    client_response_buffer buffer = clientResponseBuffer[clientID];
    client_block_t *clientBlock = client_getBlock(clientID);

    if (dispatcherState == OPENED) {
        printf("[  -  D  -  ] [  ] Sending packet to client %u\n", clientID);
        dispatcherWritingResponseForClient(clientBlock,
                                           buffer.response_to_wait,
                                           buffer.responses);
    } else {
        // if it's night, enqueue the requests
        printf("[  -  D  -  ] [  ] I'm sleeping, adding the packet for the client %u to the queue\n", clientID);
        response_node_t *node = createResponseNode(buffer.responses, buffer.response_to_wait, clientID);
        enqueueResponse(fifoForSleepyDispatcher, node);
    }
}


/**
 * Add a new responses to the client buffer. If it was the last response needed, sends it all to the client
 * (if the dispatcher isn't sleeping)
 * @param clientID The id of the client receiving a response
 * @param response The response received
 */
void addNewResponse(unsigned int clientID, client_packet_t response) {
    printf("[  -  D  -  ] [  ] Add a new response for the client %u\n", clientID);
    client_response_buffer *buffer = &clientResponseBuffer[clientID];
    buffer->responses[buffer->response_received] = response;
    buffer->response_received += 1;

    if (buffer->response_received == buffer->response_to_wait) {
        sendToClient(clientID);
    }
}


// ========= Dispatcher utility functions =========
/**
 * Manage the reception of a client packet : (called when the dispatcher is awake)
 * read the packet and send requests to or enqueue them for an appropriate desk.
 * @param packet_size The number of requests in the packet
 * @param request A pointer to the packet
 * @param client_id The ID of the client
 */
void dispatcherDealsWithClientPacket(client_packet_t *request, unsigned int client_id) {
    unsigned int desk = request->type;
    desk_block_t *desk_block = desk_getBlock(desk);

    desk_packet_t work = {
            .delay = request->delay,
            .serial_number = client_id
    };
    if (desksOccupancy[desk] == WORKING) {
        // printf("[  -  D  -  ] [  ] Desk %d is full, enqueueing\n", desk);
        node_t *node = createNode(client_id, request->type, request->delay);
        enqueue(desksQueues[desk], node);
    } else {
        // printf("[  -  D  -  ] [  ] Sending work to desk %d\n", desk);
        desksOccupancy[desk] = WORKING;
        desk_dispatcherSendsWork(desk_block, work);
    }
}

/**
 * Manage the reception of a response from a desk : (called when the dispatcher is awake)
 * mark the desk as free, add the response in a buffer for the client and
 * if the responding desk has requests remaining in its corresponding queue, send it the request.
 * @param packet A pointer to the packet
 * @param desk_id The ID (= type) of the desk
 */
void dispatcherDealsWithDeskPacket(desk_packet_t packet, unsigned int desk_id) {
    // Send the response to the client
    // printf("[  -  D  -  ] [  ] Received response from desk %d, freeing occupancy\n", desk_id);
    desksOccupancy[desk_id] = FREE;
    client_packet_t response = {
            .type = desk_id,
            .delay = packet.delay
    };
    addNewResponse(packet.serial_number, response);

    if (!isEmpty(desksQueues[desk_id])) {
        // printf("[  -  D  -  ] [  ] Queue not empty for desk %d, dequeueing\n", desk_id);
        node_t *removedNode = dequeue(desksQueues[desk_id]);
        // if it's not null, create a packet with it
        client_packet_t *newPacket = malloc(sizeof(client_packet_t));
        newPacket->type = removedNode->task;
        newPacket->delay = removedNode->delay;
        // and deal with the packet
        // printf("[  -  D  -  ] [  ] Sending dequeued packet for desk %d\n", desk_id);
        dispatcherDealsWithClientPacket(newPacket, removedNode->serial_number);
    } else {
        // printf("[  -  D  -  ] [  ] Desk %d's queue empty\n", desk_id);
    }
}

// ========= Signal handling dispatcher ===========

/**
 * Handle reception of a request signal sent by a client.
 * If the dispatcher is awake, call the method dispatcherDealsWithClientPacket.
 * Otherwise, enqueue de packet and deal with it later. (at 6am)
 */
void dispatcherHandleRequest(int signum, siginfo_t *info, void *context) {
    // Packet received from a client
    client_block_t *block = client_getBlock(info->si_value.sival_int);
    unsigned int data_size = block->data_size;
    client_packet_t *request = dispatcherGetDataFromClient(block);


    createNodeOnClientBuffer(data_size, info->si_value.sival_int);


    // if it's the day, go ahead
    if (dispatcherState == OPENED) {
        for (int i =0; i < data_size; i++) {
            dispatcherDealsWithClientPacket(&request[i], info->si_value.sival_int);
        }
    }
    else {
        // if it's night, enqueue the requests
        // printf("[  -  D  -  ] [  ] Sorry client I'm sleeping (it will be done later in the day)\n");
        node_t *node;
        for (int i = 0; i < data_size; i++) {
            node = createNode(info->si_value.sival_int, request[i].type, request[i].delay);
            enqueue(dispatcherClientQueue, node);
        }
    }
}

/**
 * Handle reception of a response signal sent by a desk :
 * read the corresponding block and call the function dispatcherDealsWithDeskPacket
 */
void dispatcherHandleResponse(int signum, siginfo_t *info, void *unused) {
    // Received response signal
    task_t task = info->si_value.sival_int;
    desk_block_t *block = desk_getBlock(task);
    desk_packet_t work = dispatcherGetResponseFromDesk(block);

    dispatcherDealsWithDeskPacket(work, info->si_value.sival_int);
}

/**
 * Nicely prints time for our tiny beautiful eyes.
 */
void printTime() {
    int hours = (dispatcherTime / 3600) % 24;
    int minutes = (dispatcherTime % 3600) / 60;
    int secs = (dispatcherTime % 3600) % 60;
    // printf("x----------x\n");
    printf("[  %02d:%02d:%02d ]\n", hours, minutes, secs);
    // printf("x----------x\n");
}

/**
 * Handles reception of a timer signal by incrementing the timer counter appropriately and updating the state of the
 * dispatcher if it is closing or opening. If it is opening, handle all requests sent by client overnight and all
 * responses sent by desks overnight.
 */
void timerSignalHandler(int signum, siginfo_t *info, void *context) {
    // Handle the timer signal
    dispatcherTime += TIMER_SCALE
    if (dispatcherTime > 86400) {
        dispatcherTime = dispatcherTime % 86400;
    }
    // If time is <= 6 am or > 6 pm
    int wasOpen = dispatcherState;
    dispatcherState = dispatcherTime >= 21600 && dispatcherTime < 64800;
    printTime();
    if (wasOpen && (dispatcherState == CLOSED)) printf("[  -  D  -  ] [  ] Bravo six, going dark\n");
    if (!wasOpen && (dispatcherState == OPENED)) {
        printf("[  -  D  -  ] [  ] GOOOOOOOOOD MORNING GAMERS\n");

        while (!isEmpty(dispatcherClientQueue)) {
            node_t *removedNode = dequeue(dispatcherClientQueue);
            client_packet_t *packet = malloc(sizeof(client_packet_t));
            packet->type = removedNode->task;
            packet->delay = removedNode->delay;
            dispatcherDealsWithClientPacket(packet, removedNode->serial_number);
        }
        // printf("[  -  D  -  ] [  ] All requests forwarded\n");

        while (!isResponseFifoEmpty(fifoForSleepyDispatcher)) {
            response_node_t *node = dequeueResponse(fifoForSleepyDispatcher);
            client_block_t *clientBlock = client_getBlock(node->clientID);
            dispatcherWritingResponseForClient(clientBlock,
                                               node->packetSize,
                                               node->packet);
        }

        // printf("[  -  D  -  ] [  ] All packet forwarded\n");
    }
}

/**
 * Handle the reception of SIGINT and SIGKILL for the dispatcher.
 * One it receive one of them, it sends SIGKILL signals to the Client and the Desk processes
 * and free the memoryHandlers.
 */
void shutDownSignalHandler(int signum, siginfo_t *info, void *context) {
    printf("[  -  D  -  ] [  ] Received shutdown signal\n");

    sigqueue(clientPID, SIGKILL, (union sigval) NULL);
    sigqueue(deskPID, SIGKILL, (union sigval) NULL);

    client_destroyMemoryHandler();
    desk_destroyMemoryHandler();
    free(dispatcherClientQueue);
    free(fifoForSleepyDispatcher);
    exit(EXIT_SUCCESS);
}

// ================== Dispatcher ==================

/**
 * Behavior of the dispatcher when first running : set the signal handlers and create queues.
 */
void dispatcher_behavior() {
    timer_t timer;
    struct sigevent sev;
    struct itimerspec its;
    memset(&descriptor, 0, sizeof(descriptor));

    descriptor.sa_flags = SA_SIGINFO;
    sigfillset(&mask);
    sigdelset(&mask, SIGINT); // I want to kill the dispatcher with CTRL+C
    sigdelset(&mask, SIGTERM);
    sigdelset(&mask, SIGKILL);
    sigprocmask(SIG_SETMASK, &mask, NULL);

    // Safe shutdown
    descriptor.sa_sigaction = shutDownSignalHandler;
    sigaction(SIGINT, &descriptor, NULL);
    sigaction(SIGTERM, &descriptor, NULL);
    sigaction(SIGKILL, &descriptor, NULL);

    // Timer
    descriptor.sa_sigaction = timerSignalHandler;
    sigdelset(&mask, TIMER_SIGNAL);
    sigprocmask(SIG_SETMASK, &mask, NULL);
    descriptor.sa_mask = mask;
    sigaction(TIMER_SIGNAL, &descriptor, NULL);

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = TIMER_SIGNAL;
    sev.sigev_value.sival_ptr = &timer;
    timer_create(CLOCK_REALTIME, &sev, &timer);

    its.it_interval.tv_sec = 1;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = 1;
    its.it_value.tv_nsec = 0;
    timer_settime(timer, 0, &its, NULL);

    // Signals for communications between dispatcher, clients and desks

    descriptor.sa_mask = mask;
    descriptor.sa_sigaction = dispatcherHandleRequest;
    sigaction(SIGRT_REQUEST, &descriptor, NULL);

    sigdelset(&mask, SIGRT_RESPONSE);

    descriptor.sa_mask = mask;
    descriptor.sa_sigaction = dispatcherHandleResponse;
    sigaction(SIGRT_RESPONSE, &descriptor, NULL);

    sigdelset(&mask, SIGRT_REQUEST);
    sigdelset(&mask, SIGRT_RESPONSE);
    sigprocmask(SIG_SETMASK, &mask, NULL);

    dispatcherClientQueue = createFifo();

    // Set all desks as free
    for (int i = 0; i < DESK_COUNT; i++) {
        desksQueues[i] = createFifo();
        desksOccupancy[i] = FREE;
    }

    fifoForSleepyDispatcher = createResponseFifo();

    while (1) {
        pause();
    }
}


int main(int argc, char const *argv[]) {
    pthread_t clients[CLIENT_COUNT];
    pthread_t desks[DESK_COUNT];

    srand(time(NULL));

    if (client_initMemoryHandler() == IPC_ERROR) {
        perror("Error while initializing memory handler");
        exit(EXIT_FAILURE);
    }

    if (desk_initMemoryHandler() == IPC_ERROR) {
        perror("Error while initializing memory handler");
        exit(EXIT_FAILURE);
    }

    dispatcherState = dispatcherTime >= 21600 && dispatcherTime < 64800;
    printTime();

    pid_t client = fork();
    if (client == 0) {
        // Créer des thread avec tous les clients
        for (int i = 0; i < CLIENT_COUNT; i++) {
            default_information_client_t *arg = malloc(sizeof(default_information_client_t));
            client_block_t *block = client_claimBlock();
            arg->block = block;
            arg->id = block->block_id;
            arg->dispatcher_id = getppid();
            pthread_create(&clients[i], NULL, client_behavior, arg);
        }

        // Attendre la fin de tous les threads
        for (int i = 0; i < CLIENT_COUNT; i++) {
            pthread_join(clients[i], NULL);
        }
        // stop the Dispatcher.
        printf("[  -  D  -  ] [  ] All the clients finished\n");
        sigqueue(getppid(), SIGINT, value);
    } else if (client > 0) {
        pid_t desk = fork();
        if (desk == 0) {
            // Créer des thread avec tous les desks
            for (int i = 0; i < DESK_COUNT; i++) {
                default_information_desk_t *arg = malloc(sizeof(default_information_desk_t));
                desk_block_t *block = desk_claimBlock();
                arg->block = block;
                arg->id = block->block_id;
                arg->dispatcher_id = getppid();
                arg->task = (task_t) i;
                pthread_create(&desks[i], NULL, desk_behavior, arg);
            }

            // Attendre la fin de tous les threads
            for (int i = 0; i < DESK_COUNT; i++) {
                pthread_join(desks[i], NULL);
            }
        } else if (desk > 0) {
            clientPID = client;
            deskPID = desk;

            dispatcher_behavior();
            client_destroyMemoryHandler();
        } else {
            perror("desk fork");
            return EXIT_FAILURE;
        }
    } else {
        perror("client fork");
        return EXIT_FAILURE;
    }
    free(dispatcherClientQueue);
    free(fifoForSleepyDispatcher);
    return EXIT_SUCCESS;
}