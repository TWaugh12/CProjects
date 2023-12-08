#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

/* Main function that sends a signal to a process
 *
 * Pre-conditions: Must be supplied a signal type and PID
 * retval: Returns 0 on successful signal send and 1 on error or incorrect usage
 */
int main(int argc, char *argv[]) {

    // Check number of arguments passed
    if (argc != 3) {
        printf("Usage: send_signal <signal type> <pid>\n");
        return 1;
    }

    // Grab PID and convert to an integer
    int pid = atoi(argv[2]);
    int signalType;

    // Determine type of signal and print error if type is invalid
    if (strcmp(argv[1], "-u") == 0) {
        signalType = SIGUSR1;
    } else if (strcmp(argv[1], "-i") == 0) {
        signalType = SIGINT;
    } else {
        printf("Invalid signal type\n");
        return 1;
    }

    // Send the specified signal to the given PID
    if (kill(pid, signalType) == -1) {

        // Check for error when sending signal
        perror("Error sending signal");
        return 1;
    }

    // If successful, return 0
    return 0;
}
