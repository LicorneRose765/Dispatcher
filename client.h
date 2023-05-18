#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>


// Idée : Chaque client (thread) aura un identifiant unique qui est en fait l'endroit où il
// va écrire ses demandes. Le dispatcher va lire les demandes de chaque client et les traiter/

// Le numéro de série de la demande est donné par le dispatcher.


// ========= Signal handling =========






// ========= Client =========

void *client_behavior(void *arg) {
    /**
     * V 1. Créer un client
     * V 2. Envoyer une demande
     * X 3. Attendre la réponse
     * V 4. Attendre un temps aléatoire
     * V 5. Retour à l'étape 2
    */

    srand(time(0));
    default_information_t *info = (default_information_t *) arg;
    pid_t dispatcher_id = info->dispatcher_id;
    char *block = info->block;
    int block_size = info->block_size;

    printf("Creating a client\n");

    pid_t id = getpid();
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
    packet->number_of_request = num_demandes;
    packet->requests = demandes;


    //sleep(temps_min);
    while (sent < NUMBER_OF_REQUESTS) {
        printf("[Client : %d] Sending request\n", id);
        //write_request(packet, block);
        sent++;

        // Send request
        printf("[Client : %d] Sending request signal\n", id);
        kill(dispatcher_id,  SIGRT_REQUEST);

        // TODO : En fait le pid d'un thread c'est le pid du processus qui l'a créé :doomed:

        pause(); // TODO : gérer les signaux pour savoir quand on a reçu la réponse
        // Read response
        printf("[Client : %d] Reading response\n", id);
        // TODO : read response
        read_request(block);


        sleep(temps_max);
    }

    free(demandes);
    free(packet);
}