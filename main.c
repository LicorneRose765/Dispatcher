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

int dispatcherIsOpen = 0;
int oneSecondsIRLEqualsHowManySeconds = 60;
long dispatcherTime = 0;

pid_t clientPID;
pid_t guichetPID;


// ========= Dispatcher buffers ==========
// Stores the id of the guiche, guichet_id[0] deals with the task 0, guichet_id[1] deals with the task 1, etc.
unsigned int guichet_i[GUICHET_COUNT];



// ========= Dispatcher utility functions =========
void DispatcherDealsWithRequest(client_packet_t *request, unsigned int client_id) {
    printf("[Dispatcher] Dealing with request from client %d\n", client_id);
    // TODO : Traiter la demande
}


// ========= Signal handling dispatcher ===========

void DispatcherHandleRequest(int signum, siginfo_t *info, void *context) {
    client_block_t *block = client_getBlock(info->si_value.sival_int);
    unsigned int data_size = block->data_size;
    client_packet_t * request = dispatcherGetDataFromClient(block);
    printf("[DISPATCHER] Received %d task from client %d \n The tasks are : \n", data_size, info->si_value.sival_int);
    for(int i=0; i<data_size; i++){
        printf("\t type : %d - delay : %ld\n", request[i].type, request[i].delay);
    }

    unsigned int guichet = request->type;
    guichet_block_t *guichet_block = guichet_getBlock(guichet);

    guichet_packet_t work = {
            .delay = request->delay,
            .serial_number = info->si_value.sival_int
    };
    printf("[DISPATCHER] Sending work to guichet %d\n", guichet);
    guichet_dispatcherSendsWork(guichet_block, work);
}

void DispatcherHandleResponse(int signum, siginfo_t *info, void *context) {
    printf("[Dispatcher] Received response from guichet\n");

    guichet_block_t *block = guichet_getBlock(info->si_value.sival_int);
    guichet_packet_t work = guichet_dispatcherGetResponseFromGuichet(block);

    // Send the response to the client
    client_block_t *client_block = client_getBlock(work.serial_number);
    client_packet_t* packet = malloc(sizeof(client_packet_t));
    packet[0].type = info->si_value.sival_int;
    packet[0].delay = work.delay;

    printf("[DISPATCHER] Sending response to client %d\n", work.serial_number);
    dispatcherWritingResponseForClient(client_block, 1, packet);
}

void timerSignalHandler(int signum, siginfo_t *info, void *context) {
    // Handle the timer signal
    printf("Received timer signal\n");
    dispatcherTime += oneSecondsIRLEqualsHowManySeconds;
    if (dispatcherTime >= 86400) {
        dispatcherTime = dispatcherTime % 4600;
    }
    // If time is <= 6 am or > 6 pm
    dispatcherIsOpen = dispatcherTime >= 21600 && dispatcherTime < 64800;
    // Handle the timer signal
    time_t currentTime = time(NULL);
    struct tm *localTime = localtime(&currentTime);
    int hour = localTime->tm_hour;
    dispatcherIsOpen = hour >= 6 && hour < 18;
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
    sigfillset(&mask);
    sigdelset(&mask, SIGINT); // I want to kill the dispatcher with CTRL+C
    sigdelset(&mask, SIGKILL);

    sigdelset(&mask, SIGRT_REQUEST);
    sigdelset(&mask, SIGRT_RESPONSE);

    sigprocmask(SIG_SETMASK, &mask, NULL);

    memset(&descriptor, 0, sizeof(descriptor));
    descriptor.sa_flags = SA_SIGINFO;

    // On masque tous les signaux sauf SIGINT pendant la gestion des request/response

    descriptor.sa_sigaction = DispatcherHandleRequest;
    sigaction(SIGRT_REQUEST, &descriptor, NULL);

    descriptor.sa_sigaction = DispatcherHandleResponse;
    sigaction(SIGRT_RESPONSE, &descriptor, NULL);

    descriptor.sa_sigaction = shutDownSignalHandler;
    sigaction(SIGINT, &descriptor, NULL);
    sigaction(SIGKILL, &descriptor, NULL);

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

            timer_t timer;
            struct sigevent sev;
            struct itimerspec its;

            struct sigaction sa;
            sa.sa_flags = SA_SIGINFO;
            sa.sa_sigaction = timerSignalHandler;
            sigdelset(&sa.sa_mask, TIMER_SIGNAL);
            sigaction(TIMER_SIGNAL, &sa, NULL);

            sev.sigev_notify = SIGEV_SIGNAL;
            sev.sigev_signo = TIMER_SIGNAL;
            sev.sigev_value.sival_ptr = &timer;
            timer_create(CLOCK_REALTIME, &sev, &timer);

            its.it_interval.tv_sec = 1;
            its.it_interval.tv_nsec = 0;
            its.it_value.tv_sec = 1;
            its.it_value.tv_nsec = 0;
            timer_settime(timer, 0, &its, NULL);

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
    return EXIT_SUCCESS;
}
