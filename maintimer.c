#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#define TIMER_SIGNAL SIGRTMIN+2





int main() {
    // Create a timer
    timer_t timer;
    struct sigevent sev;
    struct itimerspec its;

    // Set up the timer signal handler
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timerSignalHandler;
    sigemptyset(&sa.sa_mask);
    sigaction(TIMER_SIGNAL, &sa, NULL);

    // Create the timer
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = TIMER_SIGNAL;
    sev.sigev_value.sival_ptr = &timer;
    timer_create(CLOCK_REALTIME, &sev, &timer);

    // Set the timer to trigger every second
    its.it_interval.tv_sec = 1;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = 1;
    its.it_value.tv_nsec = 0;
    timer_settime(timer, 0, &its, NULL);

    // Keep the process running
    while (1) {
        // Do other tasks here
    }

    return 0;
}
