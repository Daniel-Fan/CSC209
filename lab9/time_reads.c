/* The purpose of this program is to practice writing signal handling
 * functions and observing the behaviour of signals.
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

/* Message to print in the signal handling function. */
#define MESSAGE "%ld reads were done in %ld seconds.\n"

/* Global variables to store number of read operations and seconds elapsed. 
 */
long num_reads, seconds;


void handler(int sig) {
    fprintf(stderr, MESSAGE, num_reads, seconds);
    exit(0);
}

/* The first command-line argument is the number of seconds to set a timer to run.
 * The second argument is the name of a binary file containing 100 ints.
 * Assume both of these arguments are correct.
 */

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: time_reads s filename\n");
        exit(1);
    }
    seconds = strtol(argv[1], NULL, 10);

    FILE *fp;
    if ((fp = fopen(argv[2], "r")) == NULL) {
      perror("fopen");
      exit(1);
    }
    
    struct sigaction act;
    act.sa_handler = handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGPROF,&act,NULL);
    
    struct itimerval timer;
    timer.it_value.tv_sec = seconds;
    timer.it_value.tv_usec = 0;
    timer.it_interval = timer.it_value;
    if (setitimer(ITIMER_PROF, &timer, NULL) == -1) {
        perror("error calling setitimer()");
        exit(1);
    }
    /* In an infinite loop, read an int from a random location in the file,
     * and print it to stderr.
     */
    int index;
    int dest;
    num_reads = 0;
    for (;;) {
        index = random() % 100;
        fseek(fp, sizeof(int)*index, SEEK_SET);
        fread(&dest, sizeof(int), 1, fp);
        num_reads++;
        fprintf(stderr, "%d\n", dest);
    }
    return 1; // something is wrong if we ever get here!
}
