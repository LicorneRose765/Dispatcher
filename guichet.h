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
    srand(time(0));

    default_information_guichet_t *info = (default_information_guichet_t *) arg;
    pid_t dispatcher_id = info->dispatcher_id;
    block_t *block = info->block;
    unsigned int id = info->id;
    task_t task_type = info->task;

    printf("[Guichet block id : %d] Creating a guichet with task : %d\n",id ,task_type);

    while (1) {
        // char *request = (char *) guichetWaitingRequest(block);
        // TODO : read request and do something with it
        // printf("[Guichet : %d] Received request : %s\n", id, request);

        // TODO : write response
        char *response = "Hello i'm a response from the guichet";
        //guichetWritingResponse(block, response);
        // sigqueue(dispatcher_id, SIGRT_RESPONSE, (union sigval) {.sival_int = id});
    }


}

