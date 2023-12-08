#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

// Define global variables
int alarmDuration = 5;
int sigusr1Counter = 0;

/* Handles SIGALRM signal by printing the time and PID
 *
 * Pre-conditions: SIGALRM signal must be received
 * sig: The signal number 
 * retval: void
 */
void alarmHandler(int sig) {

    // Get PID and current time to print
    pid_t pid = getpid();
    time_t currentTime = time(NULL);
    char *timeString = ctime(&currentTime);

    // Check return value of time
    if (currentTime == (time_t)(-1)) {
        perror("Failed to get the current time");
        exit(0);
    }
    
    // Check return value of ctime
    if (timeString == NULL) {
        perror("Failed to convert time to string");
        exit(EXIT_FAILURE);
    }

    // Printing PID and time to user
    printf("PID: %d CURRENT TIME: %s", pid, timeString);
    
    // Re-arm the alarm
    alarm(alarmDuration);
}

/* Handles SIGUSR1 signal by incrementing the counter
 *
 * Pre-conditions: SIGUSR1 signal must be received
 * sig: The signal number
 * retval: void
 */
void sigusr1Handler(int sig) {

    // Increments counter for SIGUSR1 and prints statement
    sigusr1Counter++;
    printf("SIGUSR1 handled and counted!\n");
}

/* Handles SIGINT signal by printing and exiting
 *
 * Pre-conditions: SIGINT signal must be received
 * sig: The signal number
 * retval: void
 */
void sigintHandler(int sig) {

    // Print message to user that SIGINT was handled
    printf("\nSIGINT handled.\n");

    // Prints message with number of times handles and exits program
    printf("SIGUSR1 was handled %d times. Exiting now.\n", sigusr1Counter);
    exit(0);
}

/* Main function that sets up signal handlers starts infinite loop
 *
 * Pre-conditions: None
 * retval: Returns 0, but program is should be be terminated by a signal before this
 */
int main() {

    // Initialize sigaction structures
    struct sigaction saAlarm, saSigusr1, saSigint;

    // Clear out the sigaction structs
    memset(&saAlarm, 0, sizeof(saAlarm));
    memset(&saSigusr1, 0, sizeof(saSigusr1));
    memset(&saSigint, 0, sizeof(saSigint));

    // Set the handler functions
    saAlarm.sa_handler = alarmHandler;
    saSigusr1.sa_handler = sigusr1Handler;
    saSigint.sa_handler = sigintHandler;

    // Register signal handlers and check return values of sigaction
    if (sigaction(SIGALRM, &saAlarm, NULL) == -1) {
        perror("Error setting SIGALRM handler");
        exit(0);
    }

    if (sigaction(SIGUSR1, &saSigusr1, NULL) == -1) {
        perror("Error setting SIGUSR1 handler");
        exit(0);
    }

    if (sigaction(SIGINT, &saSigint, NULL) == -1) {
        perror("Error setting SIGINT handler");
        exit(0);
    }

    // Set an initial alarm
    alarm(alarmDuration);

    // Print alarm frequency and user command to end program
    printf("PID and time print every %d seconds.\n", alarmDuration);
    printf("Type Ctrl-C to end the program.\n");

    // Infinite loop
    while (1) {
    }

    return 0;
}