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


    default_information_client_t *info = (default_information_client_t *) arg;
    pid_t dispatcher_id = info->dispatcher_id;
    client_block_t *block = info->block;
    unsigned int client_id = info->id;

    printf("[Client : %d] Creating a client with block %d\n", client_id, block->block_id);

    time_t temps_min = (rand() % MAX_TIME_BEFORE_REQUESTS + 1);
    time_t temps_max = (rand() % MAX_TIME_BETWEEN_REQUESTS + 1);
    int num_requests = (rand() % GUICHET_COUNT) + 1; // Minimum 1 task and max GUICHET_COUNT tasks

    // Used to share the client id in the signal
    union sigval value;
    value.sival_int = client_id;

    // Generate requests
    client_packet_t* packet = malloc(sizeof(client_packet_t) * num_requests);

    for (int i = 0; i < num_requests; i++) {
        task_t random_task = (task_t) rand() % GUICHET_COUNT;
        time_t random_delay = rand() % 10 + 1; // Minimum 1 second and max 10 seconds

        client_packet_t current_request = {
                .type = random_task,
                .delay = random_delay,
        };
        // TODO : regarder si c'est bien client_id pour le numéro de série .
        packet[i] = current_request;
    }




    clientWritingRequest(block, num_requests, packet);

    printf("[Client : %d] Sending request signal\n", client_id);
    sigqueue(dispatcher_id, SIGRT_REQUEST, value);


    client_packet_t *response = (client_packet_t *) clientWaitData(block);

    printf("[Client : %d] Received response :\n", client_id);
    for (int i = 0; i < num_requests; i++) {
        printf("\tTask : %d, delay : %ld\n", response[i].type, response[i].delay);
    }

    sleep(temps_max);


    free(packet); // TODO : why not working
}