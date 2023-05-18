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
     * Idd auprès du dispatcher
     * temp minimum et maximum avant d'introduire un nouveau paquet
     * list demandes
     *
    **/
    unsigned int id;
    // TODO : ajouter le temps min et max
    time_t temps_min;
    time_t temps_max;
    task_t *demandes;

} client_t;

typedef struct {
    /**
     * Idd auprès du dispatcher.
     * Idd du type de demande gérées.
    */
    unsigned int id;
    task_t type_demande;
} guichet_t;


typedef struct {
    /**
     * Idd du type de demande
     * Serial Number de l'instance (désigné par le dispatcher)
     * Delay : temp nécessaire pour traiter la demande (on va sleep le guichet avec ce delay)
    */
    task_t type_demande;
    int serial_number;
    time_t delay;
} demande_t;


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


int main(int argc, char const *argv[]) {
    pthread_t clients[CLIENT_COUNT];
    pthread_t guichets[GUICHET_COUNT];

    pid_t client = fork();
    char *block = load_block("application-shm", BLOCK_SIZE);
    if (block == NULL) return EXIT_FAILURE;

    if (client == 0) {
        // Créer des thread avec tous les clients
        for (int i = 0; i < CLIENT_COUNT; i++) {
            pthread_create(&clients[i], NULL, client_behavior, block);
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
            for (int i = 0; i < CLIENT_COUNT; i++) {
                pthread_create(&guichets[i], NULL, guichet_behavior, block);
            }

            // Attendre la fin de tous les threads
            for (int i = 0; i < CLIENT_COUNT; i++) {
                pthread_join(guichets[i], NULL);
            }
            printf("All guichets are dead\n");
        } else if (guichet > 0) {
            // TODO : Dispatcher behavior
            printf("I'm going to read, wait...\n");
            sleep(5);
            printf("Reading: \"%s\"\n", block);
        } else {
            perror("fork");
            return EXIT_FAILURE;
        }
    } else {
        perror("fork");
        return EXIT_FAILURE;
    }

    detach_block(block);
    return destroy_block("application-shm");
}
