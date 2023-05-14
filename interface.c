// 1. Sets up a signal handler for SIGUSR1. The handler causes the loop below to end, including sending 0 to the server. If the SIGUSR1 interrupted the system call to read from the user, that read should not be restarted.
// 2. Opens the named pipe given as the command line argument for writing.
// 3. Repeatedly (until integer is 0):
//   3.1 Prompts the user and read an integer from the terminal (you may assume that the user will enter a positive integer).
//   3.2. If the signal handler has stopped the loop, change the integer to 0.
//   3.3. Write the integer up the pipe (and flush the pipe to make sure the integer gets to the Fibonacci server).
// 4. Closes the pipe
// 5. Exits

// Here is a sample run (S: means server.c, I: means interface.c) without reaching the CPU limit:
// > Server.out 100
// S: Setting CPU limit to 100s
// I: Which Fibonacci number do you want : 44
// I: Which Fibonacci number do you want : S: Server has used 0s 1756us
// S: Received 44 from interface
// S: Created and detached the thread for 44
// 43
// I: Which Fibonacci number do you want : S: Server has used 0s 515572us
// S: Received 43 from interface
// S: Created and detached the thread for 43
// 33
// I: Which Fibonacci number do you want : S: Server has used 2s 210572us
// S: Received 33 from interface
// S: Created and detached the thread for 33
// S: Fibonacci 33 is 3524578
// 42
// I: Which Fibonacci number do you want : S: Server has used 3s 795963us
// S: Received 42 from interface
// S: Created and detached the thread for 42
// 0
// I: Interface is exiting
// S: Waiting for 3 threads
// S: Waiting for 3 threads
// S: Fibonacci 43 is 433494437
// S: Fibonacci 42 is 267914296
// S: Waiting for 1 threads
// S: Fibonacci 44 is 701408733
// S: Server has used 11s 654368us
// S: Child 3948743 completed with status 0

// Here is a sample run (S: means server.c, I: means interface.c) reaching the CPU limit:
// > Server.out 4
// S: Setting CPU limit to 4s
// I: Which Fibonacci number do you want : 44
// I: Which Fibonacci number do you want : S: Server has used 0s 1727us
// S: Received 44 from interface
// S: Created and detached the thread for 44
// 43
// I: Which Fibonacci number do you want : S: Server has used 0s 602763us
// S: Received 43 from interface
// S: Created and detached the thread for 43
// 33
// I: Which Fibonacci number do you want : S: Server has used 1s 793560us
// S: Received 33 from interface
// S: Created and detached the thread for 33
// S: Fibonacci 33 is 3524578
// 42
// I: Which Fibonacci number do you want : S: Server has used 2s 906340us
// S: Received 42 from interface
// S: Created and detached the thread for 42
// 0
// S: Received a SIGXCPU, ignoring any more
// S: Received a SIGXCPU, sending SIGUSR1 to interface
// I: Received a SIGUSR1, stopping loop
// I: Reading from user abandoned
// I: Interface is exiting
// S: Waiting for 3 threads
// S: Waiting for 3 threads
// S: Waiting for 3 threads
// S: Fibonacci 42 is 267914296
// S: Fibonacci 43 is 433494437
// S: Waiting for 1 threads
// S: Fibonacci 44 is 701408733
// S: Server has used 12s 845677us
// S: Child 3949093 completed with status 0

// Include necessary header files
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

// Global variables
int fd;
int sigusr1 = 0;

// Signal handler for SIGUSR1
void sigusr1_handler(int sig) {
  sigusr1 = 1;

  // Print out the message
  printf("I: Received a SIGUSR1, stopping loop\n");
  printf("I: Reading from user abandoned\n");
}

// Main function
int main(int argc, char *argv[]) {
  // Check if the number of arguments is correct
  if (argc != 2) {
    fprintf(stderr, "I: Usage: %s <pipe>\n", argv[0]);
    exit(1);
  }

  // Set up signal handler for SIGUSR1
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = sigusr1_handler;
  sigaction(SIGUSR1, &sa, NULL);

  // Open the named pipe given as the command line argument for writing
  fd = open(argv[1], O_WRONLY);
  if (fd == -1) {
    perror("open");
    exit(1);
  }

  // Repeatedly (until integer is 0)
  while (1) {
    // Prompts the user and read an integer from the terminal
    int integer;
    printf("I: Which Fibonacci number do you want : ");
    scanf("%d", &integer);

    // If the signal handler has stopped the loop, change the integer to 0
    if (sigusr1 == 1) {
      integer = 0;
    }

    // Write the integer up the pipe
    write(fd, &integer, sizeof(integer));

    // If the integer is 0, break out of the loop
    if (integer == 0) {
      break;
    }
  }

  // Close the pipe
  close(fd);

  // Exit
  printf("I: Interface is exiting\n");
  exit(0);
}