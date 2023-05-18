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
#include "block.h"
#include "client.h"



/******************************************************************************
* S-INFO-111 --- Solution pour Travail 1                                      *
* Groupe 03                                                                   *
* Membres:                                                                    *
* - BERNARD Thomas                                                            *
* - MOREAU Arnaud                                                             *
* - MOREAU Cyril                                                              *
******************************************************************************/

typedef struct {
    /**
     * Idd auprès du dispatcher.
     * Idd du type de demande gérées.
    */
    unsigned int id;
    task_t type_demande;
} guichet_t;


void *guichet_behavior(void *arg) {
    /**
     * 1. Créer un guichet
     * 2. Attendre une demande
     * 3. Traiter la demande
     * 4. Retour à l'étape 2
    */
    guichet_t guichet;
    guichet.id = gettid();
    guichet.type_demande = TASK1; // TODO : random
}

sigset_t mask;
struct sigaction descriptor;
union sigval value;


// ========= Signal handling dispatcher ===========

void DispatcherHandleRequest(int signum, siginfo_t *info, void *context) {
    printf("[Dispatcher] Received request from %d\n", info->si_pid);
    // TODO : Traiter la demande

}

void DispatcherHandleResponse(int signum, siginfo_t *info, void *context) {
    printf("[Dispatcher] Received response from %d\n", info->si_pid);
    // TODO : traiter la réponse
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
    char *block = load_block("application-shm", BLOCK_SIZE);
    if (block == NULL) return EXIT_FAILURE;

    if (client == 0) {
        // Créer des thread avec tous les clients
        default_information_t *info = malloc(sizeof(default_information_t));
        info->block = block;
        info->dispatcher_id = getppid();
        info->block_size = BLOCK_SIZE; // TODO : à changer

        for (int i = 0; i < CLIENT_COUNT; i++) {
            pthread_create(&clients[i], NULL, client_behavior, info);
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
                pthread_create(&guichets[i], NULL, guichet_behavior, block);
            }

            // Attendre la fin de tous les threads
            for (int i = 0; i < GUICHET_COUNT; i++) {
                pthread_join(guichets[i], NULL);
            }
            printf("All guichets are dead\n");
        } else if (guichet > 0) {
            dispatcher_behavior(guichets, clients, block);
        } else {
            perror("guichet fork");
            return EXIT_FAILURE;
        }
    } else {
        perror("client fork");
        return EXIT_FAILURE;
    }

    detach_block(block);
    return destroy_block("application-shm");
}
