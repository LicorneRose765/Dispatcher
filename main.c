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


/******************************************************************************
* S-INFO-111 --- Solution pour Travail 1                                      *
* Groupe 03                                                                   * 
* Membres:                                                                    *
* - BERNARD Thomas                                                            *
* - MOREAU Arnaud                                                             *
* - MOREAU Cyril                                                              *
******************************************************************************/

#define CLIENT_COUNT 5
#define GUICHET_COUNT 4
#define IPC_ERROR -1
#define BLOCK_SIZE 4096
#define NUMBER_OF_REQUESTS 1
typedef enum {
    TASK1, TASK2, TASK3
} task_t;

typedef struct {
    /**
     * Idd auprès du dispatcher
     * temp minimum et maximum avant d'introduire un nouveau paquet
     * list demandes
     * 
    * */
    unsigned int id;
    // TODO : ajouter le temps min et max
    time_t temps_min;
    time_t temps_max;
    task_t* demandes;

} Client;
// typedef
// struct Client client_t;


typedef struct {
    /**
     * Idd auprès du dispatcher.
     * Idd du type de demande gérées.
    */
    unsigned int id;
    task_t type_demande;
} Guichet;
// typedef
// struct Guichets guichet_t;


typedef struct {
    /**
     * Idd du type de demande
     * Serial Number de l'instance (désigné par le dispatcher)
     * Delay : temp nécessaire pour traiter la demande (on va sleep le guichet avec ce delay)
    */
    task_t type_demande;
    int serial_number;
    time_t delay;
} Demande;
// typedef
// struct Demande demande_t;


int client_behavior(char *block) {
    /**
     * 1. Créer un client
     * 2. Envoyer une demande
     * 3. Attendre la réponse
     * 4. Attendre un temps aléatoire
     * 5. Retour à l'étape 2
    */
    printf("Creating a client\n");
    int sent = 0;
    Client client;
    client.id = gettid();
    client.temps_min = 1; // TODO : random
    client.temps_max = 5; // TODO : random
    int num_demandes = 2; // TODO : random ?

    client.demandes = malloc(num_demandes * sizeof(task_t));
    for (size_t i = 0; i < num_demandes; i++) {
        // TODO : random
        if (i < num_demandes / 2) client.demandes[i] = TASK1;
        else client.demandes[i] = TASK2;
    }

    // client.demandes = {TASK1, TASK2}; // TODO : random

    while (sent < NUMBER_OF_REQUESTS) {
        // sleep(client.temps_min);
        // TODO : envoyer une demande
        strncpy(block, "I'm some data", BLOCK_SIZE);
        sent++;
        // sleep(client.temps_max);
    }

    free(client.demandes);
    return EXIT_SUCCESS;
}


int guichet_behavior() {
    /**
     * 1. Créer un guichet
     * 2. Attendre une demande
     * 3. Traiter la demande
     * 4. Retour à l'étape 2
    */
    Guichet guichet;
    guichet.id = gettid();
    guichet.type_demande = TASK1; // TODO : random

    return EXIT_SUCCESS;
}


static int get_block(char *filename, int size) {
    key_t ipc_key;
    // Try to get a key for the file
    ipc_key = ftok(filename, 4242);
    // No key = error
    if (ipc_key == IPC_ERROR) return IPC_ERROR;
    // Use the key to create/get a block ID associated with the key
    return shmget(ipc_key, size, 0644 | IPC_CREAT);
}


char *load_block(char *filename, int size) {
    int block_id = get_block(filename, size);
    char *result;
    // No block ID = error
    if (block_id == IPC_ERROR) return NULL;
    // Map the block into the process' address space
    // to get a pointer to the block using the block ID
    result = shmat(block_id, NULL, 0);
    // No result = no block
    if (result == (char *) IPC_ERROR) return NULL;
    return result;
}


int detach_block(char *block) {
    return (shmdt(block) != IPC_ERROR);
}

int destroy_block(char *filename) {
    int id = get_block(filename, 0);
    if (id == IPC_ERROR) return -1;
    return (shmctl(id, IPC_RMID, NULL) != IPC_ERROR);
}


int main(int argc, char const *argv[]) {
    pthread_t *clients[CLIENT_COUNT];
    pthread_t *guichets[GUICHET_COUNT];

    pid_t client = fork();
    char *block = load_block("application-shm", BLOCK_SIZE);
    if (block == NULL) return EXIT_FAILURE;

    if (client == 0) {
        // Créer des thread avec tous les clients
        for (int i = 0; i < CLIENT_COUNT; i++) {
            pthread_create(clients[i], NULL, client_behavior, block);
        }

        // Attendre la fin de tous les threads
        for (int i = 0; i < CLIENT_COUNT; i++) {
            pthread_join(*clients[i], NULL);
        }
        detach_block(block);
    } else if (client > 0) {
        pid_t guichet = fork();
        if (guichet == 0) {
            // TODO : guichet behavior
            // Créer des thread avec tous les guichets
            for (int i = 0; i < CLIENT_COUNT; i++) {
                pthread_create(guichets[i], NULL, guichet_behavior, block);
            }

            // Attendre la fin de tous les threads
            for (int i = 0; i < CLIENT_COUNT; i++) {
                pthread_join(*guichets[i], NULL);
            }
            detach_block(block);
        } else if (guichet > 0) {
            // TODO : Dispatcher behavior
            printf("Reading: \"%s\"\n", block);
            detach_block(block);
        } else {
            perror("fork");
            return EXIT_FAILURE;
        }
    } else {
        perror("fork");
        return EXIT_FAILURE;
    }


    return EXIT_SUCCESS;
}
