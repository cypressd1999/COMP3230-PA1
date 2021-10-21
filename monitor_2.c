#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>

// global variable to indicate whether received signal
int signal_received = 0;
void sig_handler(int signum);
void to_upper(char * lower);


int main(int argc, char *argv[]){
    // Register signal handler
    signal(SIGINT, sig_handler);
    
    // read input
    if (argc <= 1){
        return 0;
    }
    
    // create an index array to store the position of the command; for example, "./monitor cat test | grep some_word", *command = {1, 4}
    // command[i] means the the i-th command starts at command[i] position
    // the argument to be feed into the child process is argv[command[i]] to argv[command[i+1]-2], with length command[i+1] - command[i], end with NULL
    int num_command = 1;
    for (int i = 0; i < argc; i = i + 1){
        if (strcmp(argv[i], "!") == 0){
            num_command = num_command + 1;
        }
    }
    int command[num_command];
    command[0] = 1;
    int j = 1; // for iterate command
    for (int i = 0; i < argc; i = i + 1){
        if (strcmp(argv[i], "!") == 0){
            command[j] = i + 1;
            j = j + 1;
        }
    }

    // pid[] stores pid for all child process
    // start[] stores start time for all child process
    pid_t pid[num_command];
    struct timespec start[num_command], end;

    // Pipe: pipes[i * 2] is read end; pipes[i * 2 + 1] is write end
    int num_pipes = num_command - 1;
    int pipes[num_pipes * 2];
    for (int j = 0; j < num_pipes; j = j + 1){
        pipe(pipes + 2 * j);
    }

    // ::: Execument Command
    for (int i = 0; i < num_command; i = i + 1){
        // create child process
        pid[i] = fork();
        clock_gettime(CLOCK_REALTIME, &start[i]);  
        if (pid[i] < 0){ 
            fprintf(stderr, "Error: : fork failed\n");
            exit(1);
        }else if (pid[i] == 0){ // child process
            // re register default signal for child process
            signal(SIGINT,SIG_DFL);

            // Build arguments for execvp
            int num_args;
            if (i == num_command - 1) { // last command
                num_args = argc - command[i] + 1;
            }else{
                num_args = command[i+1] - command[i];
            }
            char * args[num_args];
            for (int j = 0; j < num_args - 1; j = j + 1){
                args[j] = argv[command[i] + j];
            }
            args[num_args-1] = NULL;

            printf("Process with id: %d created for the command: %s\n", (int) getpid(), args[0]);
            
            // Build pipes
            if (i == 0) { // first command
                dup2(pipes[2 * (i) + 1], 1);
                for (int m = 0; m < num_pipes * 2; m = m + 1){
                    close(pipes[m]);
                }
            }else if (i != num_command - 1){
                dup2(pipes[2 * (i-1)],0);
                dup2(pipes[2 * (i) + 1], 1);
                for (int m = 0; m < num_pipes * 2; m = m + 1){
                    close(pipes[m]);
                }
            }else{
                dup2(pipes[2 * (i-1)],0);
                for (int m = 0; m < num_pipes * 2; m = m + 1){
                    close(pipes[m]);
                }
            }

            execvp(args[0], args);
            // Error 
            fprintf(stderr, "%s error : : %s\n\nmonitor experienced an error in starting the command: %s\n", args[0], strerror(errno), args[0]);
            exit(1);
        }
    }

    for (int m = 0; m < num_pipes * 2; m = m + 1){
            close(pipes[m]);
    }

    // ::: Deal with statistics
    for (int j = 0; j < num_command; j = j + 1){
        int status;
        struct rusage usage;
        wait4(pid[j], &status, 0, &usage);
        clock_gettime(CLOCK_REALTIME, &end);
        
        // handle signal
        if (WIFSIGNALED(status)){
            int sig_num = WTERMSIG(status);
            char* sig_name = strdup(strsignal(sig_num));
            to_upper(sig_name);
            printf("\nThe command \"%s\" is interrupted by the signal number = %d (SIG%s)\n\n", argv[command[j]], sig_num, sig_name);
        }else{
            printf("\nThe command \"%s\" terminated with returned status code = %d\n\n", argv[command[j]], status);
        }
        
        // child process statistics
        float micro_unit = 1000000.0;
        float real_time = (float) (end.tv_nsec - start[j].tv_nsec) / micro_unit / (1000.0);
        float user_time = usage.ru_utime.tv_usec / micro_unit;
        float sys_time = usage.ru_stime.tv_usec / micro_unit;
        long page_faults = usage.ru_minflt + usage.ru_majflt;
        long cont_switches = usage.ru_nvcsw + usage.ru_nivcsw;
        
        printf("real: %0.3f s, user: %0.3f s, system: %0.3f s\n", real_time, user_time, sys_time);
        printf("no. of page faults: %ld\n", page_faults);
        printf("no. of context switches: %ld\n\n", cont_switches);
    }
}

void sig_handler(int signum){
    // signal_received = True
    signal_received = 1;
    // re register sinal handler, to protect the program
    signal(SIGINT,SIG_DFL);
}

void to_upper(char * lower){
    for (int i = 0; lower[i] != '\0'; i++){
        if(lower[i] >= 'a' && lower[i] <= 'z') {
         lower[i] = lower[i] - 'a' + 'A';
      }
    }    
}