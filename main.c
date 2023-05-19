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
#include "memory.h"
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

// ========= Signal handling dispatcher ===========

void DispatcherHandleRequest(int signum, siginfo_t *info, void *context) {
    printf("[Dispatcher] Received request from client %d\n", info->si_value.sival_int);
    // TODO : Traiter la demande
}

void DispatcherHandleResponse(int signum, siginfo_t *info, void *context) {
    printf("[Dispatcher] Received response from guichet\n");
    // TODO : traiter la réponse
}

void timerSignalHandler(int signum) {
    // Handle the timer signal
    printf("Received timer signal\n");
    // Handle the timer signal
    time_t currentTime = time(NULL);
    struct tm *localTime = localtime(&currentTime);
    int hour = localTime->tm_hour;
    dispatcherIsOpen = hour >= 6 && hour < 18;
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


    while(1){
        pause();
    }



    return EXIT_SUCCESS;

}


int main(int argc, char const *argv[]) {
    pthread_t clients[CLIENT_COUNT];
    pthread_t guichets[GUICHET_COUNT];

    pid_t client = fork();
    if (initMemoryHandler() == IPC_ERROR) {
        perror("Error while initializing memory handler");
        exit(EXIT_FAILURE);
    }

    if (client == 0) {
        default_information_t info = {
                .dispatcher_id = getppid()
        };
        // Créer des thread avec tous les clients
        for (int i = 0; i < CLIENT_COUNT; i++) {
            default_information_t *arg = malloc(sizeof(default_information_t));
            memcpy(arg, &info, sizeof(default_information_t));
            block_t *block = claimBlock();
            arg->block = block;
            arg->id = block->block_id;
            pthread_create(&clients[i], NULL, client_behavior, arg);
        }

        // Attendre la fin de tous les threads
        for (int i = 0; i < CLIENT_COUNT; i++) {
            pthread_join(clients[i], NULL);
        }
        printf("All clients are dead\n");
    } else if (client > 0) {
        pid_t guichet = fork();
        if (guichet == 0) {
            // TODO : guichet behavior
            // Créer des thread avec tous les guichets
            for (int i = 0; i < GUICHET_COUNT; i++) {
                pthread_create(&guichets[i], NULL, guichet_behavior, NULL);
            }

            // Attendre la fin de tous les threads
            for (int i = 0; i < GUICHET_COUNT; i++) {
                pthread_join(guichets[i], NULL);
            }
            printf("All guichets are dead\n");
        } else if (guichet > 0) {
            timer_t timer;
            struct sigevent sev;
            struct itimerspec its;

            struct sigaction sa;
            sa.sa_flags = SA_SIGINFO;
            sa.sa_sigaction = timerSignalHandler;
            sigemptyset(&sa.sa_mask);
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
        } else {
            perror("guichet fork");
            return EXIT_FAILURE;
        }
    } else {
        perror("client fork");
        return EXIT_FAILURE;
    }

    destroyMemoryHandler();
    return EXIT_SUCCESS;
}
