#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>

task_t get_random_task_type() {
    srand(time(NULL));
    int random_num = rand() % 3;
    task_t random_task = (task_t) random_num;
    return random_task;
}


void *guichet_behavior(void *arg) {
    /**
     * V 1. Créer un guichet
     * X 2. Attendre une demande
     * X 3. Traiter la demande
     * X 4. Retour à l'étape 2
    */

    default_information_t *info = (default_information_t *) arg;
    pid_t dispatcher_id = info->dispatcher_id;
    char *block = info->block;
    // int block_size = info->block_size;

    printf("Creating a guichet\n");

    pid_t id = gettid();
    task_t type = get_random_task_type();


}

