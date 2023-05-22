#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

#include "utils.h"
#include "fifo.h"
#include "responseFifo.h"
#include "memoryClient.h"
#include "memoryGuichet.h"
#include "client.h"
#include "guichet.h"
#include <errno.h>

/******************************************************************************
* S-INFO-111 --- Solution pour Travail 1                                      *
* Groupe 03                                                                   *
* Membres:                                                                    *
* - BERNARD Thomas                                                            *
* - MOREAU Arnaud                                                             *
* - MOREAU Cyril                                                              *
******************************************************************************/

sigset_t mask;
struct sigaction descriptor;
union sigval value;

long dispatcherTime = STARTING_TIME;
int dispatcherIsOpen = 0;

pid_t clientPID;
pid_t guichetPID;

Fifo *desksQueues[GUICHET_COUNT];
int desksOccupancy[GUICHET_COUNT];


// ========= Dispatcher buffers & function for buffers ==========
// Keep track of the state of the desks
Fifo *dispatcherClientQueue;
response_fifo *fifoForSleepyDispatcher;

client_response_buffer clientResponseBuffer[CLIENT_COUNT];

/**
 * Creates a buffer to wait for all the responses before sending the whole packet to the client
 * @param numberOfResponse The number of responses to wait
 * @param clientID The id of the client.
 */
void createNodeOnClientBuffer(unsigned int numberOfResponse, unsigned int clientID){
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
void sendToClient(unsigned int clientID){
    client_response_buffer buffer = clientResponseBuffer[clientID];
    client_block_t *clientBlock = client_getBlock(clientID);

    if (dispatcherIsOpen) {
        printf("[  -  D  -  ] [  ] Sending packet to client %u\n", clientID);
        dispatcherWritingResponseForClient(clientBlock,
                                           buffer.response_to_wait,
                                           buffer.responses);
    }
    else {
        // if it's night, enqueue the requests
        // printf("[  -  D  -  ] [  ] Sorry desk I'm sleeping (it will be done later in the day)\n");
        printf("[  -  D  -  ] [  ] I'm sleeping, adding the packet for the client %u to the queue\n", clientID);
        response_node_t *node = createResponseNode(buffer.responses, buffer.response_to_wait, clientID);
        enqueueResponse(fifoForSleepyDispatcher, node);
    }
}


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
void DispatcherDealsWithClientPacket(unsigned int packet_size, client_packet_t *request, unsigned int client_id) {
    unsigned int guichet = request->type;
    guichet_block_t *guichet_block = guichet_getBlock(guichet);

    guichet_packet_t work = {
            .delay = request->delay,
            .serial_number = client_id
    };
    if (desksOccupancy[guichet] == WORKING) {
        // printf("[  -  D  -  ] [  ] Desk %d is full, enqueueing\n", guichet);
        node_t *node = createNode(client_id, request->type, request->delay, packet_size);
        enqueue(desksQueues[guichet], node);
    } else {
        // printf("[  -  D  -  ] [  ] Sending work to guichet %d\n", guichet);
        desksOccupancy[guichet] = WORKING;
        guichet_dispatcherSendsWork(guichet_block, work);
    }
}

void DispatcherDealsWithGuichetPacket(guichet_packet_t packet, unsigned int guichet_id) {
    // Send the response to the client
    // printf("[  -  D  -  ] [  ] Received response from guichet %d, freeing occupancy\n", guichet_id);
    desksOccupancy[guichet_id] = FREE;
    client_packet_t response = {
            .type = guichet_id,
            .delay = packet.delay
    };
    addNewResponse(packet.serial_number, response);

    if (!isEmpty(desksQueues[guichet_id])) {
        // printf("[  -  D  -  ] [  ] Queue not empty for desk %d, dequeueing\n", guichet_id);
        node_t *removedNode = dequeue(desksQueues[guichet_id]);
        // if it's not null, create a packet with it
        client_packet_t *newPacket = malloc(sizeof(client_packet_t) * removedNode->packet_size);
        newPacket->type = removedNode->task;
        newPacket->delay = removedNode->delay;
        // and deal with the packet
        // printf("[  -  D  -  ] [  ] Sending dequeued packet for desk %d\n", guichet_id);
        DispatcherDealsWithClientPacket(removedNode->packet_size, newPacket, removedNode->serial_number);
    } else {
        // printf("[  -  D  -  ] [  ] Desk %d's queue empty\n", guichet_id);
    }
}

// ========= Signal handling dispatcher ===========

void DispatcherHandleRequest(int signum, siginfo_t *info, void *context) {
    // Packet reveived from client
    client_block_t *block = client_getBlock(info->si_value.sival_int);
    unsigned int data_size = block->data_size;
    client_packet_t *request = dispatcherGetDataFromClient(block);


    createNodeOnClientBuffer(data_size, info->si_value.sival_int);


    // if it's day, go ahead
    if (dispatcherIsOpen) DispatcherDealsWithClientPacket(data_size, request, info->si_value.sival_int);
    else {
        // if it's night, enqueue the requests
        // printf("[  -  D  -  ] [  ] Sorry client I'm sleeping (it will be done later in the day)\n");
        node_t *node;
        for (int i = 0; i < data_size; i++) {
            node = createNode(info->si_value.sival_int, request[i].type, request[i].delay,
                              data_size);
            enqueue(dispatcherClientQueue, node);
        }
    }
}

void DispatcherHandleResponse(int signum, siginfo_t *info, void *context) {
    // Received response signal
    task_t task = info->si_value.sival_int;
    guichet_block_t *block = guichet_getBlock(task);
    guichet_packet_t work = guichet_dispatcherGetResponseFromGuichet(block);

    DispatcherDealsWithGuichetPacket(work, info->si_value.sival_int);
}

void printTime() {
    int hours = (dispatcherTime / 3600) % 24;
    int minutes = (dispatcherTime % 3600) / 60;
    int secs = (dispatcherTime % 3600) % 60;
    // printf("x----------x\n");
    printf("[  %02d:%02d:%02d ]\n", hours, minutes, secs);
    // printf("x----------x\n");
}

void timerSignalHandler(int signum, siginfo_t *info, void *context) {
    // Handle the timer signal
    dispatcherTime += TIMER_SCALE;
    if (dispatcherTime > 86400) {
        dispatcherTime = dispatcherTime % 86400;
    }
    // If time is <= 6 am or > 6 pm
    int wasOpen = dispatcherIsOpen;
    dispatcherIsOpen = dispatcherTime >= 21600 && dispatcherTime < 64800;
    printTime();
    if (wasOpen && !dispatcherIsOpen) printf("[  -  D  -  ] [  ] Bravo six, going dark\n");
    if (!wasOpen && dispatcherIsOpen) {
        printf("[  -  D  -  ] [  ] GOOOOOOOOOD MORNING GAMERS\n");

        while (!isEmpty(dispatcherClientQueue)) {
            node_t *removedNode = dequeue(dispatcherClientQueue);
            client_packet_t *packet = malloc(sizeof(client_packet_t));
            packet->type = removedNode->task;
            packet->delay = removedNode->delay;
            DispatcherDealsWithClientPacket(removedNode->packet_size, packet, removedNode->serial_number);
        }
        // printf("[  -  D  -  ] [  ] All requests forwarded\n");

        while(!isResponseFifoEmpty(fifoForSleepyDispatcher)) {
            response_node_t *node = dequeueResponse(fifoForSleepyDispatcher);
            client_block_t *clientBlock = client_getBlock(node->clientID);
            dispatcherWritingResponseForClient(clientBlock,
                                               node->packetSize,
                                               node->packet);
        }

        // printf("[  -  D  -  ] [  ] All packet forwarded\n");
    }
}

void shutDownSignalHandler(int signum, siginfo_t *info, void *context) {
    // Handle the shutdown signal
    printf("Received shutdown signal\n");

    sigqueue(clientPID, SIGKILL, (union sigval) NULL);
    sigqueue(guichetPID, SIGKILL, (union sigval) NULL);

    client_destroyMemoryHandler();
    guichet_destroyMemoryHandler();
    exit(EXIT_SUCCESS);
}

// ================== Dispatcher ==================

int dispatcher_behavior(pthread_t *guichets, pthread_t *clients, char *block) {
    /**
     * The dispatcher needs :
     * 1. A list of guichets
     * 2. A list of clients
     * 3. A list of requests in progress
     * ...
     */
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

    // Signals for communications between dispatcher, clients and guichets

    descriptor.sa_mask = mask;
    descriptor.sa_sigaction = DispatcherHandleRequest;
    sigaction(SIGRT_REQUEST, &descriptor, NULL);

    sigdelset(&mask, SIGRT_RESPONSE);

    descriptor.sa_mask = mask;
    descriptor.sa_sigaction = DispatcherHandleResponse;
    sigaction(SIGRT_RESPONSE, &descriptor, NULL);

    sigdelset(&mask, SIGRT_REQUEST);
    sigdelset(&mask, SIGRT_RESPONSE);
    sigprocmask(SIG_SETMASK, &mask, NULL);

    dispatcherClientQueue = createFifo();

    for (int i = 0; i < GUICHET_COUNT; i++) {
        desksQueues[i] = createFifo();
        desksOccupancy[i] = FREE;
    }

    fifoForSleepyDispatcher = createResponseFifo();

    while (1) {
        pause();
    }

    return EXIT_SUCCESS;

}


int main(int argc, char const *argv[]) {
    pthread_t clients[CLIENT_COUNT];
    pthread_t guichets[GUICHET_COUNT];

    srand(time(NULL));

    if (client_initMemoryHandler() == IPC_ERROR) {
        perror("Error while initializing memory handler");
        exit(EXIT_FAILURE);
    }

    if (guichet_initMemoryHandler() == IPC_ERROR) {
        perror("Error while initializing memory handler");
        exit(EXIT_FAILURE);
    }

    dispatcherIsOpen = dispatcherTime >= 21600 && dispatcherTime < 64800;
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
        printf("All the clients finished\n");
        sigqueue(getppid(), SIGINT, value);
    } else if (client > 0) {
        pid_t guichet = fork();
        if (guichet == 0) {
            // Créer des thread avec tous les guichets
            for (int i = 0; i < GUICHET_COUNT; i++) {
                default_information_guichet_t *arg = malloc(sizeof(default_information_guichet_t));
                guichet_block_t *block = guichet_claimBlock();
                arg->block = block;
                arg->id = block->block_id;
                arg->dispatcher_id = getppid();
                arg->task = (task_t) i;
                pthread_create(&guichets[i], NULL, guichet_behavior, arg);
            }

            // Attendre la fin de tous les threads
            for (int i = 0; i < GUICHET_COUNT; i++) {
                pthread_join(guichets[i], NULL);
            }
            // printf("All guichets are dead\n");
        } else if (guichet > 0) {
            clientPID = client;
            guichetPID = guichet;

            dispatcher_behavior(guichets, clients, NULL);
            client_destroyMemoryHandler();
        } else {
            perror("guichet fork");
            return EXIT_FAILURE;
        }
    } else {
        perror("client fork");
        return EXIT_FAILURE;
    }
    free(dispatcherClientQueue);
    return EXIT_SUCCESS;
}