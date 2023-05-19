#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "block.h"



// ========= Client =========

void *client_behavior(void *arg) {
    /**
     * V 1. Créer un client
     * V 2. Envoyer une demande
     * V 3. Attendre la réponse
     * V 4. Attendre un temps aléatoire
     * V 5. Retour à l'étape 2
    */

    srand(time(0));

    default_information_t *info = (default_information_t *) arg;
    pid_t dispatcher_id = info->dispatcher_id;
    block_t *block = info->block;
    unsigned int id = info->id;


    time_t temps_min = (rand() % MAX_TIME_BEFORE_REQUESTS + 1);
    time_t temps_max = (rand() % MAX_TIME_BETWEEN_REQUESTS + 1);
    task_t *demandes;
    int num_demandes = (rand() % 3) + 1;

    int sent = 0;

    demandes = malloc(num_demandes * sizeof(task_t));
    for (size_t i = 0; i < num_demandes; i++) {
        demandes[i] = (task_t) (rand() % 3);
    }

    packet_request_t *packet = malloc(sizeof(packet_request_t));
    packet->client_id = id;
    packet->number_of_request = num_demandes;
    packet->requests = demandes;

    // Used to share the client id in the signal
    union sigval value;
    value.sival_int = id;


    //sleep(temps_min);
    while (sent < NUMBER_OF_REQUESTS) {
        printf("[Client : %d] Sending request\n", id);
        char *request = "Hello i'm a request";
        clientWritingRequest(block, request);
        sent++;

        // Send request
        printf("[Client : %d] Sending request signal\n", id);
        sigqueue(dispatcher_id, SIGRT_REQUEST, value);

        char *data = clientWaitingResponse(block);
        // Read response
        printf("[Client : %d] Reading response = %s\n", id, data);

        sleep(temps_max);
    }

    free(demandes);
    free(packet);
}