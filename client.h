#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>



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

    default_information_client_t *info = (default_information_client_t *) arg;
    pid_t dispatcher_id = info->dispatcher_id;
    block_t *block = info->block;
    unsigned int client_id = info->id;


    time_t temps_min = (rand() % MAX_TIME_BEFORE_REQUESTS + 1);
    time_t temps_max = (rand() % MAX_TIME_BETWEEN_REQUESTS + 1);
    int num_requests = (rand() % GUICHET_COUNT) + 1; // Minimum 1 task and max GUICHET_COUNT tasks

    // Used to share the client id in the signal
    union sigval value;
    value.sival_int = client_id;

    // Generate requests
    srand(time(NULL));
    request_group_t *packet = malloc(sizeof(request_group_t));

    packet->requests = malloc(sizeof(request_t) * num_requests);

    for (int i = 0; i < num_requests; i++) {
        task_t random_task = (task_t) rand() % GUICHET_COUNT;
        time_t random_delay = rand() % 10 + 1; // Minimum 1 second and max 10 seconds

        request_t current_request = {
                .type = random_task,
                .delay = random_delay,
                .serial_number = client_id
        };
        // TODO : regarder si c'est bien client_id pour le numéro de série .
        packet->requests[i] = current_request;
    }

    // Put the requests in a packet
    packet->client_id = client_id;
    packet->num_requests = num_requests;
    printf("num_requests = %d\n", num_requests);
    printf("packet is built on the house\n");

    // Write the packet in the shm
    printf("[Client : %d] Sending requests packet\n", client_id);
    clientWritingRequest(block, packet);
    sigqueue(dispatcher_id, SIGRT_REQUEST, value);
    printf("[Client : %d] Sending request signal\n", client_id);

    // char *data = clientWaitingResponse(block);
    // Read response
    // printf("[Client : %d] Reading response = %s\n", client_id, data);

    sleep(temps_max);

    /*
    //sleep(temps_min);
    while (sent < NUMBER_OF_REQUESTS) {

        printf("[Client : %d] Sending request\n", id);
        char *request = "Hello i'm a request";
        clientWritingRequest(block, request);
        sent++;

        // Send request
        printf("[Client : %ud] Sending request signal\n", id);
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