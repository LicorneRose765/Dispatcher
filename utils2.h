#define NUM_CLIENTS 3
#define NUM_DESKS 3

#define MIN_REQUESTS 1
#define MAX_REQUESTS 5

#define MIN_SLEEP 1
#define MAX_SLEEP 10

// Returned values
#define IPC_ERROR (-1)
#define WRITING_SUCCESS 1
#define WRITING_ERROR (-1)

typedef enum {
    TASK1, TASK2, TASK3
} task_t;


typedef struct {
    task_t type;
    int serial_number;
    int delay;
} request_t;


typedef struct {
    task_t type;
    int serial_number;
} response_t;
