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
    // read input
    if (argc <= 1){
        return 0;
    }
    char* args[argc];
    for (int i = 0; i < argc-1; i = i + 1){
        args[i] = argv[i+1];
    }
    args[argc-1] = NULL;
    
    // create child process
    int status;
    struct rusage usage;
    struct timespec start, end;
    pid_t pid = fork();
    clock_gettime(CLOCK_REALTIME, &start);
    if (pid < 0){ 
        fprintf(stderr, "Error: : fork failed\n");
        exit(1);
    }else if (pid == 0){ // child process
        printf("Process with id: %d created for the command: %s\n", (int) getpid(), args[0]);
        // deal with 

        // sleep(300000);

        execvp(args[0], args);
        // Error 
        printf("exec: : %s", strerror(errno));
        printf("\n\nmonitor experienced an error in starting the command: %s\n", args[0]);
        exit(1);
    }else{ // parent process
        // Register signal handler
        signal(SIGINT, sig_handler);
        wait4(pid, &status, 0, &usage);
        clock_gettime(CLOCK_REALTIME, &end);
        
        // handle signal
        if (WIFSIGNALED(status)){
            int sig_num = WTERMSIG(status);
            char* sig_name = strdup(sys_signame[sig_num]);
            to_upper(sig_name);
            printf("The command \"%s\" is interrupted by the signal number = %d (SIG%s)\n\n", args[0], sig_num, sig_name);
        }else{
            printf("The command \"%s\" terminated with returned status code = %d\n\n", args[0], status);
        }
        
        // child process statistics
        float micro_unit = 1000000.0;
        float real_time = (float) (end.tv_nsec - start.tv_nsec) / micro_unit / (1000.0);
        float user_time = usage.ru_utime.tv_usec / micro_unit;
        float sys_time = usage.ru_stime.tv_usec / micro_unit;
        long page_faults = usage.ru_minflt + usage.ru_majflt;
        long cont_switches = usage.ru_nvcsw + usage.ru_nivcsw;
        
        printf("real: %0.3f s, user: %0.3f s, system: %0.3f s\n", real_time, user_time, sys_time);
        printf("no. of page faults: %ld\n", page_faults);
        printf("no. of context switches: %ld\n", cont_switches);
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