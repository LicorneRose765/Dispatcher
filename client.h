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
    // task_t *demandes;
    int num_requests = (rand() % 5) + 1; // TODO : ça ou bien le #define NUMBER_OF_REQUESTS ?

    // Used to share the client id in the signal
    union sigval value;
    value.sival_int = id;

    // Generate requests
    srand(time(NULL));
    request_group_t packet;
    printf("created packet, will contain %i requests\n", num_requests);

    for (int i = 0; i < num_requests; i++) {
        task_t random_task = (task_t) rand() % 3;
        time_t random_delay = rand() % 10 + 1;
        printf("created random task and delay\n");

        request_t current_request = {random_task, i, random_delay};
        packet.requests[i] = &current_request;
        printf("assigned packet[%d] to a request\n", i);
    }

    // Put the requests in a packet
    packet.client_id = 1; // TODO : wer client id
    packet.num_requests = num_requests;
    printf("packet is built on the house\n");

    // Write the packet in the shm
    printf("[Client : %d] Sending requests packet\n", id);
    clientWritingRequest(block, &packet);
    printf("[Client : %d] Sending request signal\n", id);
    sigqueue(dispatcher_id, SIGRT_REQUEST, value);

    char *data = clientWaitingResponse(block);
    // Read response
    printf("[Client : %d] Reading response = %s\n", id, data);

    sleep(temps_max);

    /*
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
     */

    // free(demandes);
    // free(packet); // TODO : why not working
}