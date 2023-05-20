/**
 * Stocke les constantes et les structures utiles pour le projet
 */


// Returned values
#define IPC_ERROR (-1)
#define WRITING_SUCCESS 1
#define WRITING_ERROR (-1)

// Semaphore values
#define SEM_PRIVATE 0
#define SEM_PUBLIC 1

// Params of the application
#define CLIENT_COUNT 1
#define GUICHET_COUNT 1
#define NUMBER_OF_REQUESTS 1
#define MAX_TIME_REQUEST 600 // 10 minutes
#define MIN_TIME_REQUEST 60 // 1 minute
#define MAX_TIME_BETWEEN_REQUESTS 2
#define MAX_TIME_BEFORE_REQUESTS 2

// Signals
#define SIGRT_REQUEST (SIGRTMIN + 1)
#define SIGRT_RESPONSE (SIGRTMIN + 0)
#define TIMER_SIGNAL SIGRTMAX


typedef enum {
    TASK1, TASK2, TASK3
} task_t;

typedef struct {
    unsigned int block_id;
    sem_t semaphore;

    void* data; // TODO : à changer
} block_t;

typedef struct {
    pid_t dispatcher_id;
    block_t *block;
    unsigned int id;
} default_information_t;

typedef struct {
    task_t type;
    int serial_number;
    time_t delay;
} request_t;

typedef struct {
    unsigned int client_id;
    request_t* requests[5];
    int num_requests;
} request_group_t;

