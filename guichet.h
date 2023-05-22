/*
 * This file implements the behavior of the desks.
 * Each desk is defined by its ID which is his task type. (desk u deals with task u)
 *
 *
 * Each desk works this way :
 *      - Wait for work sent from the dispatcher
 *      - Check the delay required to do that work
 *      - Wait the delay (simulate the work)
 *      - Push the response to his block.
 *      - Signal the dispatcher with a SIGRT_RESPONSE
 *      - Repeat
 */

/**
 * Behavior of the desk process when first running : set basic info such as ID and the corresponding shared memory block
 * then follow the desk's behavior.
 * @param arg default_information_guichet_t
 */
void *guichet_behavior(void *arg) {
    srand(time(0));

    default_information_guichet_t *info = (default_information_guichet_t *) arg;
    pid_t dispatcher_id = info->dispatcher_id;
    guichet_block_t *block = info->block;
    unsigned int guichet_id = info->id;

    union sigval value;
    value.sival_int = guichet_id;


    while (1) {
        // Wait for work
        guichet_packet_t work = (guichet_packet_t) guichet_waitForWork(block);
        printf("[  -  -  G  ] [%02d] Received work, working for : %ldh\n", guichet_id, work.delay);

        sleep(work.delay);

        // send the response to the dispatcher.
        // The response is the same packet, but we can easily add data to it if want had to.

        guichet_sendResponse(block, work);
        printf("[  -  -  G  ] [%02d] Sending response\n", guichet_id);
        sigqueue(dispatcher_id, SIGRT_RESPONSE,  value);
    }


}

