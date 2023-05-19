#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>


// En gros chaque thread aura un ID auprès du dispatcher et de son processus parent, cet
// id va permettre au processus parent de mettre à disposition les données relatives à
// la demande du client dans un tableau de données partagées entre les threads.

// Un thread attendra qu'un semaphore soit disponible pour pouvoir accéder à son slot dans le tableau


// Utiliser une structure de donnée spéciale pour stocker les données, comme ça chaque client n'a
// accès qu'à sa structure personnelle. (Genre faire un système custom)

// ========= Response message queue ========
typedef struct {
    int client_id;
    int number_of_request;
    task_t *requests;
    sem_t *semaphore;
} buffer;

buffer *mkBuffer(int client_id) {
    buffer *b = malloc(sizeof(buffer));
    b->client_id = client_id;
    sem_init(b->semaphore, 0, 0); // On initialise le semaphore à 0 parce que le buffer est vide.
    return b;
}

void freeBuffer(buffer *b) {
    free(b->requests);
    sem_destroy(b->semaphore);
    free(b);
}

int buffer_write(buffer *b, packet_request_t *packet) {
    b->number_of_request = packet->number_of_request;
    b->requests = packet->requests;
    sem_post(b->semaphore);
    return WRITING_SUCCESS;
}

// TODO : regarder si faudra pas utiliser un truc de packet spécial pour les réponses.

int buffer_read(buffer *b, packet_request_t *packet) {
    sem_wait(b->semaphore);
    packet->number_of_request = b->number_of_request;
    packet->requests = b->requests;
    return WRITING_SUCCESS;
}


// ========= Signal handling =========


void testHandler(int signum, siginfo_t *info, void *context) {
    printf("[Client process] Received signal from dispatcher %d for the %d process\n", info->si_pid, info->si_value.sival_int);
    pthread_sigqueue(info->si_value.sival_int, SIGRT_RESPONSE, info->si_value);

}


void threadResponseHandler(int signum, siginfo_t *info, void *context) {
    printf("[Client %d] Received response from Dispacther\n", gettid());
}





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
    int id = info->id;

    printf("Creating client %d\n", id);

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
        //write_request(packet, block);
        sent++;

        // Send request
        printf("[Client : %d] Sending request signal\n", id);
        sigqueue(dispatcher_id, SIGRT_REQUEST, value);

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