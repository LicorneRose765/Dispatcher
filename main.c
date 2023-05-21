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
#include "memoryClient.h"
#include "memoryGuichet.h"
#include "client.h"
#include "guichet.h"



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
Fifo *dispatcherClientQueue;
Fifo *dispatcherDeskQueue;

pid_t clientPID;
pid_t guichetPID;


// ========= Dispatcher buffers ==========
// TODO : create here the queues for the dispatcher


// ========= Dispatcher utility functions =========
void DispatcherDealsWithClientPacket(unsigned int packet_size, client_packet_t *request, unsigned int client_id) {
    printf("[DISPATCHER] Received %d task from client %d \n The tasks are : \n", packet_size, client_id);
    for(int i=0; i<packet_size; i++){
        printf("\t type : %d - delay : %ld\n", request[i].type, request[i].delay);
    }

    unsigned int guichet = request->type;
    guichet_block_t *guichet_block = guichet_getBlock(guichet);

    guichet_packet_t work = {
            .delay = request->delay,
            .serial_number = client_id
    };
    printf("[DISPATCHER] Sending work to guichet %d\n", guichet);
    guichet_dispatcherSendsWork(guichet_block, work);
}

void DispatcherDealsWithGuichetPacket(guichet_packet_t packet, unsigned int guichet_id) {
    // Send the response to the client
    client_block_t *client_block = client_getBlock(packet.serial_number);
    client_packet_t* response = malloc(sizeof(client_packet_t));
    response[0].type = guichet_id;
    response[0].delay = packet.delay;

    printf("[DISPATCHER] Sending response to client %d\n", packet.serial_number);
    dispatcherWritingResponseForClient(client_block, 1, response);
}

// ========= Signal handling dispatcher ===========

void DispatcherHandleRequest(int signum, siginfo_t *info, void *context) {
    client_block_t *block = client_getBlock(info->si_value.sival_int);
    unsigned int data_size = block->data_size;
    client_packet_t * request = dispatcherGetDataFromClient(block);
    // if it's day, go ahead
    if (dispatcherIsOpen) DispatcherDealsWithClientPacket(data_size, request, info->si_value.sival_int);
    else {
        // if it's night, enqueue the requests
        printf("[Dispatcher] Sorry client I'm sleeping (it will be done later in the day)\n");
        node_t *node;
        for (int i = 0; i < data_size; i++) {
            node = createNode(info->si_value.sival_int, request[i].type, request[i].delay, data_size);  // TODO serial number = client_id ?
            enqueue(dispatcherClientQueue, node);
        }
    }
}

void DispatcherHandleResponse(int signum, siginfo_t *info, void *context) {
    printf("[Dispatcher] Received response from guichet\n");

    guichet_block_t *block = guichet_getBlock(info->si_value.sival_int);
    guichet_packet_t work = guichet_dispatcherGetResponseFromGuichet(block);

    if (dispatcherIsOpen) DispatcherDealsWithGuichetPacket(work, info->si_value.sival_int);
    else {
        // if it's night, enqueue the requests
        printf("[Dispatcher] Sorry desk I'm sleeping (it will be done later in the day)\n");
        node_t *node = createNode(info->si_value.sival_int, 0, work.delay, 1);  // TODO task = NULL ?
        enqueue(dispatcherDeskQueue, node);
    }
}

void printTime() {
    int hours = (dispatcherTime / 3600) % 24;
    int minutes = (dispatcherTime % 3600) / 60;
    int secs = (dispatcherTime % 3600) % 60;
    printf("  o  [Clock] %02d:%02d:%02d\n", hours, minutes, secs);
}

void timerSignalHandler(int signum, siginfo_t *info, void *context) {
    // Handle the timer signal
    // printf("============================ Received timer signal\n");
    dispatcherTime += TIMER_SCALE;
    if (dispatcherTime > 86400) {
        dispatcherTime = dispatcherTime % 86400;
    }
    // If time is <= 6 am or > 6 pm
    int wasOpen = dispatcherIsOpen;
    dispatcherIsOpen = dispatcherTime >= 21600 && dispatcherTime < 64800;
    printTime();
    if (wasOpen && !dispatcherIsOpen) printf("[Dispatcher] Bravo six, going dark\n");
    if (!wasOpen && dispatcherIsOpen) {
        printf("[Dispatcher] GOOOOOOOOOD MORNING GAMERS\n");
        // get the first node
        node_t *removedNode = dequeue(dispatcherClientQueue);
        if (removedNode != NULL) {
            // if it's not null, create a packet with it
            client_packet_t *packet = malloc(sizeof(client_packet_t));
            packet->type = removedNode->task;
            packet->delay = removedNode->delay;
            // and deal with the packet
            DispatcherDealsWithClientPacket(removedNode->packet_size, packet, removedNode->serial_number);
        }
        // repeat
        while (!isEmpty(dispatcherClientQueue)) {
            removedNode = dequeue(dispatcherClientQueue);
            client_packet_t *packet = malloc(sizeof(client_packet_t));
            packet->type = removedNode->task;
            packet->delay = removedNode->delay;
            DispatcherDealsWithClientPacket(removedNode->packet_size, packet, removedNode->serial_number);
        }
        printf("[Dispatcher] All requests forwarded\n"); // TODO : not taking into account that some desks could be full

        // get the first node
        removedNode = dequeue(dispatcherDeskQueue);
        if (removedNode != NULL) {
            // if it's not null, create a packet with it
            guichet_packet_t *packet = malloc(sizeof(guichet_packet_t));
            packet->delay = removedNode->delay;
            // and deal with the packet
            DispatcherDealsWithGuichetPacket(*packet, removedNode->serial_number);
        }
        // repeat
        while (!isEmpty(dispatcherDeskQueue)) {
            removedNode = dequeue(dispatcherDeskQueue);
            guichet_packet_t *packet = malloc(sizeof(guichet_packet_t));
            packet->delay = removedNode->delay;
            DispatcherDealsWithGuichetPacket(*packet, removedNode->serial_number);
        }
        free(removedNode);
        printf("[Dispatcher] All responses forwarded\n");
    }
}

void shutDownSignalHandler(int signum, siginfo_t *info, void *context) {
    // Handle the shutdown signal
    printf("Received shutdown signal\n");

    sigqueue(clientPID, SIGKILL, (union sigval) NULL);
    sigqueue(guichetPID, SIGKILL, (union sigval) NULL);

    client_destroyMemoryHandler();
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
    dispatcherDeskQueue = createFifo();

    while(1) {
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
    } else if (client > 0) {
        pid_t guichet = fork();
        if (guichet == 0) {
            // Créer des thread avec tous les guichets
            for (int i = 0; i < GUICHET_COUNT; i++) {
                default_information_guichet_t *arg = malloc(sizeof(default_information_guichet_t ));
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
            printf("All guichets are dead\n");
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
    free(dispatcherDeskQueue);
    return EXIT_SUCCESS;
}
