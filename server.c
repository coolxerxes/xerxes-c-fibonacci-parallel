// 1. Creates a named pipe, named "FIBOPIPE" (if the pipe already exists, you can use it, or delete it before creating a new one)
// 2 Sets up a signal handler for SIGXCPU. The handler ignores any further SIGXCPU signals, and sends a SIGUSR1 to the child process. If the SIGXCPU interrupted the system call to read off the pipe, that read should not be restarted.
// 3. Limits its CPU usage to the number of seconds given as the command line parameter.
// 4. Set a global counter of the number running threads 0.
// 5. Creates a child process.
//   5.1. The child process runs the user interface program described below, passing the named pipe's name in as the command line parameter.
// 6. Opens the named pipe for reading.
// 7. Repeatedly (until the integer is 0):
//   7.1. Reports the CPU time used (not including the user interface time)
//   7.2. Reads an integer N off the pipe (the integers are placed there by the user interface program).
//   7.3. Starts and detaches a new thread that ...
//     7.3.1. Calculates the Nth Fibonacci number using the recursive O(2n) algorithm.
//     7.3.2. Prints out the result
//     7.3.3. Decrements the number of running threads
//   7.4. Increment number of running threads
// 8. Closes the pipe
// 9. Waits for number of running threads to get down to 0
// 10. Reports the CPU time used (not including the user interface time)
// 11. Cleans up the user interface program zombie
// 12. Deletes the named pipe
// 13. Exits

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

// Include files
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

// Define macros
#define MAXLINE 80
#define MAXTHREADS 100

// Define global variables
int runningThreads = 0;
int pipefd;
char *pipeName;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Define signal handler
void sigHandler(int sig) {
  // Ignore further SIGXCPU signals
  signal(SIGXCPU, SIG_IGN);

  // Send SIGUSR1 to child process
  kill(0, SIGUSR1);

  printf("S: Received a SIGXCPU, sending SIGUSR1 to interface\n");
}

// Define fibonacci function
long Fibonacci(long WhichNumber) {
  if (WhichNumber <= 1) {
    return(WhichNumber);
  } else {
    return(Fibonacci(WhichNumber - 2) + Fibonacci(WhichNumber - 1));
  }
}

// Define thread function
void *threadFunction(void *arg) {
  // Get the number from the argument
  long number = (long)arg;

  // Calculate the fibonacci number
  long fibonacciNumber = Fibonacci(number);

  // Print the fibonacci number
  printf("S: %ld\n", fibonacciNumber);

  // Decrement the number of running threads
  pthread_mutex_lock(&mutex);
  runningThreads--;
  pthread_mutex_unlock(&mutex);

  // Exit the thread
  pthread_exit(NULL);
}

// Define main function
int main(int argc, char *argv[]) {
  // Check for correct number of arguments
  if (argc != 2) {
    fprintf(stderr, "S: Usage: %s <CPU limit in seconds>\n", argv[0]);
    exit(1);
  }

  // Set CPU limit
  int cpuLimit = atoi(argv[1]);
  struct rlimit cpuLimitStruct;
  cpuLimitStruct.rlim_cur = cpuLimit;
  cpuLimitStruct.rlim_max = cpuLimit;
  setrlimit(RLIMIT_CPU, &cpuLimitStruct);
  printf("S: Setting CPU limit to %ds\n", cpuLimit);

  // Create the named pipe
  pipeName = "FibonacciPipe";
  mkfifo(pipeName, 0666);

  // Create the child process
  pid_t pid = fork();

  // Check for error
  if (pid < 0) {
    fprintf(stderr, "S: Error: fork() failed\n");
    exit(1);
  }

  // Child process
  if (pid == 0) {
    // Execute the interface program
    execl("./interface", "interface", pipeName, NULL);

    // Check for error
    fprintf(stderr, "S: Error: execlp() failed\n");
    exit(1);
  }

  // Open the named pipe for reading
  pipefd = open(pipeName, O_RDONLY);

  // Check for error
  if (pipefd < 0) {
    fprintf(stderr, "S: Error: open() failed\n");
    exit(1);
  }

  // Set signal handler for SIGXCPU
  signal(SIGXCPU, sigHandler);

  // Loop until the integer is not 0
  int number = 1;
  while (number != 0) {
    // Report the CPU time used
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);

    printf("S: Server has used %ld %ds\n", usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);

    // Read an integer N off the pipe
    int readResult = read(pipefd, &number, sizeof(number));

    // Check for error
    if (readResult < 0) {
      fprintf(stderr, "S: Error: read() failed\n");
      exit(1);
    }

    // Check for end of file
    if (readResult == 0) {
      break;
    }

    // Check for SIGXCPU
    if (readResult == -1 && errno == EAGAIN) {
      break;
    }

    // Check for SIGUSR1
    if (readResult == -1 && errno == EINTR) {
      break;
    }

    // Check for error
    if (readResult != sizeof(number)) {
      fprintf(stderr, "S: Error: read() failed\n");
      exit(1);
    }

    // Check for 0
    if (number == 0) {
      break;
    }

    // Print the number
    printf("S: Received %d from interface\n", number);

    // Create a detached thread
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_mutex_lock(&mutex);
    runningThreads++;
    pthread_mutex_unlock(&mutex);
    pthread_create(&thread, &attr, threadFunction, (void *)(size_t)number);
    printf("S: Created and detached the thread for %d\n", number);
  }

  // Close the pipe
  close(pipefd);

  // Wait for all threads to complete
  while (runningThreads > 0) {
    printf("S: Waiting for %d threads\n", runningThreads);
    sleep(1);
  }

  // Wait for the child process to complete
  int status;
  waitpid(pid, &status, 0);
  printf("S: Child %d completed with status %d\n", pid, status);

  // Remove the named pipe
  unlink(pipeName);

  // Exit the program
  return(0);
}