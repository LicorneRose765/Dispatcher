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
 * @param arg default_information_desk_t
 */
void *desk_behavior(void *arg) {
    default_information_desk_t *info = (default_information_desk_t *) arg;
    pid_t dispatcher_id = info->dispatcher_id;
    desk_block_t *block = info->block;
    unsigned int desk_id = info->id;

    union sigval value;
    value.sival_int = desk_id;


    while (1) {
        // Wait for work
        desk_packet_t work = (desk_packet_t) desk_waitForWork(block);
        printf("[  -  -  G  ] [%02d] Received work, working for : %ldh\n", desk_id, work.delay);

        sleep(work.delay);

        // send the response to the dispatcher.
        // The response is the same packet, but we can easily add data to it if want had to.

        desk_sendResponse(block, work);
        printf("[  -  -  G  ] [%02d] Sending response\n", desk_id);
        sigqueue(dispatcher_id, SIGRT_RESPONSE,  value);
    }


}

