/*
 * This file implements the behavior of the clients.
 * Each client is defined by
 *      - its ID (the id of its block)
 *      - Delay before the first packet
 *      - Delay between packets
 *      - The tasks in each packet
 *
 *
 * The clients work this way :
 *      - Initialise all their information
 *      - Wait delay before first packet
 *      - Push the packet to his block
 *      - Signal the dispatcher
 *      - Wait the response of the dispatcher
 *      - Wait delay between packets
 *      - Resend a packet and repeat
 */

/**
 * Behavior of the desk process when first running : set basic info such as ID and the corresponding shared memory
 * block, the minimum and maximum waiting time and the number of requests then follow the client's behavior.
 * @param arg default_information_guichet_t
 */
void *client_behavior(void *arg) {
    default_information_client_t *info = (default_information_client_t *) arg;
    pid_t dispatcher_id = info->dispatcher_id;
    client_block_t *block = info->block;
    unsigned int client_id = info->id;


    time_t min_time = (rand() % MAX_TIME_BEFORE_REQUESTS + 1);
    time_t max_time = (rand() % MAX_TIME_BETWEEN_REQUESTS + 1);
    int num_requests = (rand() % GUICHET_COUNT) + 1; // Minimum 1 task and max GUICHET_COUNT tasks

    // Used to share the client id in the signal
    union sigval value;
    value.sival_int = client_id;

    unsigned int packetCounter = 0;

    sleep(min_time);
    while(packetCounter < MAX_PACKET_SENT) {
        // Generate requests
        client_packet_t *packet = malloc(sizeof(client_packet_t) * num_requests);

        printf("[  C  -  -  ] [%02d] Sending packet with %d tasks.\n                 | The tasks are :\n", client_id, num_requests);
        for (int i = 0; i < num_requests; i++) {
            task_t random_task = (task_t) rand() % GUICHET_COUNT;
            time_t random_delay = rand() % 5 + 1; // Minimum 1 hour and max 5 hours

            client_packet_t current_request = {
                    .type = random_task,
                    .delay = random_delay,
            };
            packet[i] = current_request;
            printf("                 |--  Type : %d, delay = %ld\n", random_task, random_delay);
        }

        packetCounter++;
        // Push packet
        clientWritingRequest(block, num_requests, packet);

        // Signal the dispatcher
        sigqueue(dispatcher_id, SIGRT_REQUEST, value);

        // Wait for the response
        client_packet_t *response = (client_packet_t *) clientWaitData(block);

        printf("[  C  -  -  ] [%02d] Received response :\n", client_id);
        for (int i = 0; i < num_requests; i++) {
            printf("                 |--  Type : %d, delay : %ld\n", response[i].type, response[i].delay);
        }

        // Wait delay between packet
        sleep(max_time);
    }
    pthread_exit(EXIT_SUCCESS);


}