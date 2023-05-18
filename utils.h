// Stocke les constantes et les structures utiles pour le projet

#define CLIENT_COUNT 2
#define GUICHET_COUNT 4
#define IPC_ERROR (-1)
#define WRITING_SUCCESS 1
#define WRITING_ERROR (-1)
#define NUMBER_OF_REQUESTS 2

// Params of the application
#define MAX_TIME_REQUEST 600 // 10 minutes
#define MIN_TIME_REQUEST 60 // 1 minute
#define MAX_TIME_BETWEEN_REQUESTS 600 // 10 minute
#define MAX_TIME_BEFORE_REQUESTS 600 // 10 minutes

// Signals
#define SIGRT_REQUEST (SIGRTMIN + 1)
#define SIGRT_RESPONSE (SIGRTMIN + 0)


typedef enum {
    TASK1, TASK2, TASK3
} task_t;

typedef struct {
    pid_t dispatcher_id;
    char *block;
    int block_size; // Not sure of the block size
} default_information_t;


typedef struct {
    int number_of_request;
    task_t *requests;
} packet_request_t;

typedef struct {
    task_t task;
    int serial_number;
    time_t delay;
} request_t;

