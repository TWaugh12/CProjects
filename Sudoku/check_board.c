#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *DELIM = ","; 

/*   
 * Read the first line of input file to get the size of that board.
 * 
 * PRE-CONDITION #1: file exists
 * PRE-CONDITION #2: first line of file contains valid non-zero integer value
 *
 * fptr: file pointer for the board's input file
 * size: a pointer to an int to store the size
 *
 * POST-CONDITION: the integer whos address is passed in as size (int *) 
 * will now have the size (number of rows and cols) of the board being checked.
 */
void get_board_size(FILE *fptr, int *size) {      
   char *line1 = NULL;
   size_t len = 0;

   if ( getline(&line1, &len, fptr) == -1 ) {
      printf("Error reading the input file.\n");
      free(line1);
      exit(1);
   }

   char *size_chars = NULL;
   size_chars = strtok(line1, DELIM);
   *size = atoi(size_chars);

   // free memory allocated for reading first link of file
   free(line1);
   line1 = NULL;
}



/* 
 * Returns 1 if and only if the board is in a valid Sudoku board state.
 * Otherwise returns 0.
 * 
 * A valid row or column contains only blanks or the digits 1-size, 
 * with no duplicate digits, where size is the value 1 to 9.
 * 
 * board: heap allocated 2D array of integers 
 * size:  number of rows and columns in the board
 */
int valid_board(int **board, int size) {

    // Initialize pointers to arrays that store information about whether number has been seen
    int *row_storage = (int*)malloc(size * sizeof(int));  
    int *col_storage = (int*)malloc(size * sizeof(int));

    // Checks library function malloc return value and handles error
    if (row_storage == NULL || col_storage == NULL) {
        printf("Malloc failed \n");
        exit(1);
    }

    // Begin iteration over the rows of the board
    for (int i = 0; i < size; i++) {   

    // Reset the information about rows and columns for board check
        for (int j = 0; j < size; j++) {
            *(row_storage + j) = 0;
            *(col_storage + j) = 0;
        }

        // Check rows for duplicate numbers
        for (int j = 0; j < size; j++) {

            // Set temporary variable equal to number in board position i, j
            int row_temp = *(*(board + i) + j);
            if (row_temp != 0) {

                // Checks if number has been seen through storage
                if (*(row_storage + row_temp - 1) == 1) {
                    
                    // If it has free memory and return 0
                    free(row_storage);
                    free(col_storage);
                    return 0;
                }

                // Store that number has been seen
                *(row_storage + row_temp - 1) = 1;
            }

            // Set temporary variable equal to number in board position j, i
            int col_temp = *(*(board + j) + i);

            // Check columns for duplicate numbers
            if (col_temp != 0) {

                // Checks if number has been seen through storage
                if (*(col_storage + col_temp - 1) == 1) {
 
                    // If it has free memory and return 0
                    free(row_storage);
                    free(col_storage);
                    return 0;  
                }

                // Store that number has been seen
                *(col_storage + col_temp - 1) = 1;  
            }
        }
    }

    // Free memory and return 1
    free(row_storage);
    free(col_storage);
    return 1;
}


/* 
 * This program prints "valid" (without quotes) if the input file contains
 * a valid state of a Sudoku puzzle board wrt to rows and columns only.
 *
 * A single CLA is required, which is the name of the file 
 * that contains board data is required.
 *
 * argc: the number of command line args (CLAs)
 * argv: the CLA strings, includes the program name
 *
 * Returns 0 if able to correctly output valid or invalid.
 * Only exit with a non-zero result if unable to open and read the file given.
 */
int main( int argc, char **argv ) {              

   // Check if number of command-line arguments is correct.
   if (argc != 2) {
	   printf("Incorrect number of arguments");
	   return 1;
   }
   // Open the file and check if it opened successfully.
   FILE *fp = fopen(*(argv + 1), "r");
   if (fp == NULL) {
      printf("Can't open file for reading.\n");
      exit(1);
   }

   // Declare local variables.
   int size;

   // Call get_board_size to read first line of file as the board size
   get_board_size(fp, &size);

   // Dynamically allocate a 2D array for given board size.
   int **board = malloc(size * sizeof(int*));
   if (!board) {
       printf("Malloc failed \n");
       fclose(fp);
       exit(1);
   } 

   // Iterate through rows and allocate memory accordingly
   for (int i = 0; i < size; i++) {
      *(board+i)  = (int*)malloc(size * sizeof(int));

      // Check for malloc errors while allocating memory
      if (!*(board+i)) {
          printf("Malloc failed \n");

          // Free previously allocated memory and exit
          for (int j = 0; j < 1; j++) {
              free(*(board + j));
          }
          free(board);
          exit(1);
      }
   }
   // Read the rest of the file line by line.

   char *line = NULL;
   size_t len = 0;
   char *token = NULL;

   for (int i = 0; i < size; i++) {
      
      // Check for errors when reading file and print message
      if (getline(&line, &len, fp) == -1) {
         printf("Error while reading line %i of the file.\n", i+2);
         exit(1);
      }

      // Tokenize csv file using ',' DELIM for reading
      token = strtok(line, DELIM);

      for (int j = 0; j < size; j++) {
    
    
         // Convert token to int and fill in 2D array
         *(*(board + i) + j) = atoi(token);
         token = strtok(NULL, DELIM);
      }
   }

   // Call the function valid_board and print the appropriate
   //       output depending on the function's return value.
   if (valid_board(board, size)) {
      printf("valid\n");
   } else {
       printf("invalid\n");
   }
   //  Free all dynamically allocated memory.
   for (int **p = board; p < size + board; p++) {
       free(*p);
   }

   free(board);
   free(line);

   //Close the file
   if (fclose(fp) != 0) {
      printf("Error while closing the file.\n");
      exit(1);
   } 

   return 0;       
}       

