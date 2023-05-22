/*
 * Stores the constants and the structures used in this project.
 */


// Returned values
#define IPC_ERROR (-1)

// Semaphore values
#define SEM_PRIVATE 0
#define SEM_PUBLIC 1

// Signals
#define SIGRT_REQUEST (SIGRTMIN + 0)
#define SIGRT_RESPONSE (SIGRTMIN + 1)
#define TIMER_SIGNAL SIGRTMAX

// DESK states
#define FREE 0
#define WORKING 1

// Dispatcher states
#define OPENED 1
#define CLOSED 0

// Params of the application
#define CLIENT_COUNT 3
#define DESK_COUNT 5
#define MAX_PACKET_SENT 2 // Number of packet sent by each client before exit

#define MAX_TIME_REQUEST 12 // Max delay for a task
#define MIN_TIME_REQUEST 1 // Min delay for a task
#define MAX_TIME_BETWEEN_REQUESTS 12
#define MAX_TIME_BEFORE_REQUESTS 16

// TIMER
#define TIMER_SCALE 3600; // 1 sec = 1h in the app
#define STARTING_TIME (3600*3) // Timer starts at 3am


typedef unsigned int task_t;

// ======= Client ======

typedef struct {
    task_t type;
    time_t delay;
} client_packet_t;

typedef struct {
    unsigned int block_id;
    sem_t semaphore;
    unsigned int data_size;
    client_packet_t data[DESK_COUNT];
} client_block_t;

typedef struct {
    pid_t dispatcher_id;
    client_block_t *block;
    unsigned int id;
} default_information_client_t;

// ======== DESK =======

typedef struct {
    unsigned int serial_number;
    time_t delay;
} desk_packet_t;

typedef struct {
    unsigned int block_id;
    sem_t semaphore;
    desk_packet_t data;
} desk_block_t;

typedef struct {
    pid_t dispatcher_id;
    desk_block_t *block;
    unsigned int id;
    task_t task;
} default_information_desk_t;


// ====== Dispatcher ======

// Used to store the clients packet and waits for all the responses.
typedef struct {
    unsigned int response_to_wait;
    unsigned int response_received;
    client_packet_t *responses;
} client_response_buffer;
