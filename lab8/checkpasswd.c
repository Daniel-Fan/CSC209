#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXLINE 256
#define MAX_PASSWORD 10

#define SUCCESS "Password verified\n"
#define INVALID "Invalid password\n"
#define NO_USER "No such user\n"

int main(void) {
  char user_id[MAXLINE];
  char password[MAXLINE];
  int status;
  int pipe_fd[2];

  if(fgets(user_id, MAXLINE, stdin) == NULL) {
      perror("fgets");
      exit(1);
  }
  if(fgets(password, MAXLINE, stdin) == NULL) {
      perror("fgets");
      exit(1);
  }

  if ((pipe(pipe_fd))== -1){
      perror("pipe");
      exit(1);
  }

  int result = fork();

  if(result < 0){
      perror("fork");
      exit(1);
  }else if(result == 0){
      if(close(pipe_fd[1]) == -1){
          perror("closing write in child");
      }
      if (dup2(pipe_fd[0], fileno(stdin)) == -1) {
			    perror("dup2");
		  }
      if(close(pipe_fd[0]) == -1){
          perror("closing read in child");
      }
      execl("./validate", "validate", NULL);
      perror("exec");
		  exit(1);
  }else{
      if(close(pipe_fd[0]) == -1){
        perror("closeing read in parent");
      }
      if(write(pipe_fd[1], user_id, MAX_PASSWORD) == -1){
        perror("writing user_id to pipe");
      }
      if(write(pipe_fd[1], password, MAX_PASSWORD) == -1){
        perror("writing user_id to pipe");
      }
      if(close(pipe_fd[1]) == -1){
        perror("closing write in parent");
      }
      if(wait(&status) == -1){
          perror("wait");
          exit(1);
      }
      if(WIFEXITED(status)){
          if(WEXITSTATUS(status) == 0){
              printf("%s", SUCCESS);
          }else if(WEXITSTATUS(status) == 1){
              exit(1);
          }else if(WEXITSTATUS(status) == 2){
              printf("%s", INVALID);
          }else if(WEXITSTATUS(status) == 3){
              printf("%s",NO_USER);
          }
      }
  }
  return 0;
}
