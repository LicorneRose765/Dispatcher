#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>


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
    guichet_block_t *block = info->block;
    unsigned int guichet_id = info->id;
    task_t task_type = info->task;

    // Used to share the client id in the signal
    union sigval value;
    value.sival_int = guichet_id;

    // printf("[Guichet block id : %d] Creating a guichet with task : %d\n",guichet_id ,task_type);

    while (1) {
        guichet_packet_t work = (guichet_packet_t) guichet_waitForWork(block);
        printf("[  -  -  G  ] [%02d] Received work, working for : %ldh\n", guichet_id, work.delay);

        sleep(work.delay);

        // send the response to the dispatcher. The response is the same packet, but we can easily add data to it.
        //work.delay = (time_t) NULL;

        guichet_sendResponse(block, work);
        printf("[  -  -  G  ] [%02d] Sending response\n", guichet_id);
        sigqueue(dispatcher_id, SIGRT_RESPONSE,  value);
    }


}

