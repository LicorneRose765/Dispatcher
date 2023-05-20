/******************************************************************************
* S-INFO-111 --- Solution pour Travail 2                                      *
* Groupe 03                                                                   *
* Membres:                                                                    *
* - BERNARD Thomas                                                            *
* - MOREAU Arnaud                                                             *
* - MOREAU Cyril                                                              *
******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "utils2.h"
#include "block2.h"

void dispatcherProcess() {
    // Code du dispatcher
    printf("I'm the dispatcher process.\n");
    // Logique du dispatcher
}

void *client_behavior(void *arg) {
    request_t *requests = (request_t *)block;

    srand(time(NULL));  // Initialisation du générateur de nombres aléatoires

    int num_requests = rand() % (MAX_REQUESTS - MIN_REQUESTS + 1) + MIN_REQUESTS;

    request_t requests[num_requests];

    for (int i = 0; i < num_requests; i++) {
        int type_index = rand() % 3;
        switch (type_index) {
            case 0:
                requests[i].type = TASK1;
                break;
            case 1:
                requests[i].type = TASK2;
                break;
            case 2:
                requests[i].type = TASK3;
                break;
        }
        requests[i].serial_number = i + 1; // TODO : numéro unique
        requests[i].delay = rand() % 5 + 1;

        // Affichage des informations de la demande
        printf("Demande %d - Type: %d, Numéro de série: %d, Délai: %d\n",
               i + 1, requests[i].type, requests[i].serial_number, requests[i].delay);

        // Calculate the offset for each request
        size_t offset = i * sizeof(request_t);
        // Write the request to the block at the calculated offset
        memcpy(block + offset, &requests[i], sizeof(request_t));
    }

    int sleep_duration = rand() % (MAX_SLEEP - MIN_SLEEP + 1) + MIN_SLEEP;
    sleep(sleep_duration);

    printf("Please read my requests :(\n");

    // Read and print the requests from the block
    for (int i = 0; i < 5; i++) {
        request_t request;
        memcpy(&request, &requests[i], sizeof(request_t));

        printf("Request %d - Type: %d, Serial Number: %d, Delay: %d\n",
               i + 1, request.type, request.serial_number, request.delay);
    }

    detach_block(block);
    destroy_block("shared_memory");

    return NULL;

}

int clientProcess() {
    printf("I'm the client process.\n");
    pthread_t threads[NUM_CLIENTS];
    int i, result;

    for (i = 0; i < NUM_CLIENTS; i++) {
        result = pthread_create(&threads[i], NULL, client_behavior, NULL);
        if (result != 0) {
            printf("Erreur lors de la création du client %d\n", i);
            return -1;
        }
    }

    // Attendre la fin de tous les threads
    for (i = 0; i < NUM_CLIENTS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}

void *desk_behavior(void *arg) {
    // Comportement du thread client
    // ...
    printf("I'm a new desk !\n");
    return NULL;
}

int deskProcess() {
    printf("I'm the desk process.\n");
    pthread_t threads[NUM_DESKS];
    int i, result;

    for (i = 0; i < NUM_DESKS; i++) {
        result = pthread_create(&threads[i], NULL, desk_behavior, NULL);
        if (result != 0) {
            printf("Erreur lors de la création du desk %d\n", i);
            return -1;
        }
    }

    // Attendre la fin de tous les threads
    for (i = 0; i < NUM_DESKS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}

int main() {
    /**
     * Fork n°1 : dispatcher et autre
     * Fork n°2 : client et autre
     * Fork n°3 : desk et processus initial qui attend la terminaison de tout le monde
     *
     * INITIAL
     *  |
     *  |-- dispatcher
     *  |
     *  |----|-- client
     *       |
     *       |----|-- desk
     */
    pid_t dispatcherPID, clientPID, deskPID;

    // Création du processus dispatcher
    dispatcherPID = fork();

    if (dispatcherPID == 0) {
        // Processus dispatcher
        dispatcherProcess();
        exit(0);
    } else if (dispatcherPID > 0) {
        // Processus initial
        // Création du processus client
        clientPID = fork();

        if (clientPID == 0) {
            // Processus client
            clientProcess();
            exit(0);
        } else if (clientPID > 0) {
            // Processus initial
            // Création du processus desk
            deskPID = fork();

            if (deskPID == 0) {
                // Processus desk
                deskProcess();
                exit(0);
            } else if (deskPID > 0) {
                // Processus initial
                // Attente de la terminaison de tous les processus enfants
                waitpid(dispatcherPID, NULL, 0);
                waitpid(clientPID, NULL, 0);
                waitpid(deskPID, NULL, 0);
            } else {
                // Erreur lors de la création du processus desk
                perror("Erreur lors de la création du processus desk.");
                exit(1);
            }
        } else {
            // Erreur lors de la création du processus client
            perror("Erreur lors de la création du processus client.");
            exit(1);
        }
    } else {
        // Erreur lors de la création du processus dispatcher
        perror("Erreur lors de la création du processus dispatcher.");
        exit(1);
    }

    return 0;
}