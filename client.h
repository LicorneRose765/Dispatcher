#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>


// Dans ce fichier, on aura les fonctions de communication entre les clients et le dispatcher
// Un 'client' est un processus qui va générer un ensemble de thread correspondant à un ensemble
// de client. Ce processus va communiquer avec le dispatcher via des messages IPC et des signaux.

// Lorsque le thread d'un client veut envoyer une demande au dispatcher, il va envoyer un message IPC au dispatcher
// a


void *client_behavior(void *arg) {
    /**
     * 1. Créer un client
     * 2. Envoyer une demande
     * 3. Attendre la réponse
     * 4. Attendre un temps aléatoire
     * 5. Retour à l'étape 2
    */
    char *block = (char *) arg;
    printf("Creating a client\n");
    // TODO : randomize temps_min, temps_max and the number of requests
    time_t temps_min;
    time_t temps_max;
    task_t *demandes;
    int num_demandes = 2;

    int sent = 0;

    demandes = malloc(num_demandes * sizeof(task_t));
    for (size_t i = 0; i < num_demandes; i++) {
        // TODO : random
        if (i < num_demandes / 2) demandes[i] = TASK1;
        else demandes[i] = TASK2;
    }

    // sleep(client.temps_min);
    while (sent < NUMBER_OF_REQUESTS) {
        // TODO : envoyer une demande
        printf("Writing some data\n");
        write_to_block("I'm some data", block, 'a');
        sent++;
        // sleep(client.temps_max);
    }

    free(demandes);
}