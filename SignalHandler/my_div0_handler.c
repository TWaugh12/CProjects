#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

// Global variable for successful division count
int successfulDivisions = 0;

/* Handles SIGFPE signal when divison by zero is attempted
 *
 * Pre-conditions: SIGFPE signal must be received
 * signal: The signal number
 * retval: void
 */
void handle_sigfpe(int signal) {

    // Prints messages to user including number of successful operations and exits program
    printf("Error: a division by 0 operation was attempted.\n");
    printf("Total number of operations completed successfully: %d\n", successfulDivisions);
    printf("The program will be terminated.\n");
    exit(0);
}

/* Handles SIGINT signal
 *
 * Pre-conditions: SIGINT signal must be received
 * signal: The signal number
 * retval: void
 */
void handle_sigint(int sig) {

    // Prints messages to user including number of successful operations and exits program
    printf("Total number of operations completed successfully: %d\n", successfulDivisions);
    printf("The program will be terminated.\n");
    exit(0);
}
/* Main function of the program that runs loop and grabs user input
 *
 * Pre-conditions: None
 * retval: Returns 0, but program should be be terminated by a signal before this
 */
int main() {

    // Set buffer size and sigaction struct
    char buffer[100];
    struct sigaction sa;

    // Setup SIGFPE handler and check return value of sigaction
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigfpe;
    if (sigaction(SIGFPE, &sa, NULL) == -1) {
        perror("Error setting SIGALRM handler");
        exit(0);
    }

    // Setup SIGINT handler and check return value of sigaction
    sa.sa_handler = handle_sigint;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error setting SIGINT handler");
        exit(0);
    }

    // Begin loop 
    while (1) {

        // Prompt for first input
        printf("Enter first integer: ");

        // Grab input and check return value of fgets
        if (fgets(buffer, 100, stdin) == NULL) {
            perror("Error reading input for first integer");
            continue;
        }

        // Convert input to integer
        int num1 = atoi(buffer);

        // Prompt for second input
        printf("Enter second integer: ");
        
        // Grab second input and check return value of fgets
        if (fgets(buffer, 100, stdin) == NULL) {
            perror("Error reading input for first integer");
            continue;
        }

        // Convert input to integer
        int num2 = atoi(buffer);

        // If we are dividing by zero or are given letters, raise corresponding signal
        if (num2 == 0) {
            raise(SIGFPE); 
            continue;
        }

        // Calculate quotient and remainder
        int quotient = num1 / num2;
        int remainder = num1 % num2;

        // Increment successfulDivisions counter
        successfulDivisions++;

        // Print calculation to user
        printf("%d / %d is %d with a remainder of %d\n", num1, num2, quotient, remainder);
    }

    return 0; 
}
