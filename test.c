#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main(int argc, char *argv[]){
    pid_t pid[3];
    for (int i = 0; i < 3; i++) {
        pid[i] = fork();
        if (pid[i] == 0) {
            // getpid gives process id
            // getppid gives parent process id
            printf("child pid %d from the"
                   " parent pid %d\n",
                   getpid(), getppid());
  
            // Set Normal termination of
            // the program
            break;
        }
    }

    
  
    