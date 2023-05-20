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
#define GUICHET_COUNT 5 /* /!\ must be equal to the number of possible tasks */
#define MAX_PACKET_SEND 5

#define MAX_DATA_SIZE 5
#define MAX_BLOCK_SIZE (2 * sizeof(int) + (GUICHET_COUNT * sizeof(request_t)))
// On a 2 entier dans le bloc + un tableau de maximum GUICHET_COUNT requêtes
// TODO : peut-être changer ici (enlever le commentaire sinon)

#define MAX_TIME_REQUEST 600 // 10 minutes
#define MIN_TIME_REQUEST 60 // 1 minute
#define MAX_TIME_BETWEEN_REQUESTS 2
#define MAX_TIME_BEFORE_REQUESTS 2

// Signals
#define SIGRT_REQUEST (SIGRTMIN + 1)
#define SIGRT_RESPONSE (SIGRTMIN + 0)
#define TIMER_SIGNAL SIGRTMAX


typedef unsigned int task_t;

// ======= Client ======

typedef struct {
    task_t type;
    time_t delay;
} request_t;

typedef struct {
    unsigned int block_id;
    sem_t semaphore;
    unsigned int data_size;
    request_t data[GUICHET_COUNT];
} client_block_t;

typedef struct {
    pid_t dispatcher_id;
    client_block_t *block;
    unsigned int id;
} default_information_client_t;

// ======== Guichet =======

typedef struct {
    unsigned int serial_number;
    time_t delay;
} work_t;

typedef struct {
    unsigned int block_id;
    sem_t semaphore;
    work_t data;
} guichet_block_t;

typedef struct {
    pid_t dispatcher_id;
    guichet_block_t *block;
    unsigned int id;
    task_t task;
} default_information_guichet_t;


// Communication structures

