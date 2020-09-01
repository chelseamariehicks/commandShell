/**********************************************************************************
 * Name: Chelsea Marie Hicks
 * 
 * Description: Program creates a shell in C to run command line instructions and
 *      return results similar to other familiar shells like bash and Bourne. This
 *      shell has built-in commands cd, status, and exit, and allows for redirection
 *      of standard input and output and supports both foreground and background
 *      processes (controlled via the command line and signals received).
***********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

//Defines constants for the maximumm length and arguments command line supports
#define MAXLENGTH 2048
#define MAXARGS 512
#define CONSTSIZE 256

//Global tracker for whether fgMode only is turned on by Ctrl-Z
bool fgMode = false;

//Function prompts user to enter a command and puts the command into an array
void getCommand(char* commandArr[], bool* background, int processID, char inFile[], char outFile[]) {
    //Get command from user and store in userInput
    char userInput[MAXLENGTH];
    printf(": ");
    //Flush output buffer each time printf is used
    fflush(stdout);
    fgets(userInput, MAXLENGTH, stdin);

    //Replace newline character with null
    for(int i = 0; i < MAXLENGTH; i++) {
        if(userInput[i] == '\n') {
            userInput[i] = '\0';
        }
    }
    //If user just hit enter and no command, return that
    if(strcmp(userInput, "") == 0) {
        commandArr[0] = strdup("");
        return;
    }

    //Using strtok() to get each individual piece of user entered data
    //https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/
    char* argument = strtok(userInput, " ");

    //Place individual strings in commandArr, removing spaces
    for(int i = 0; argument; i++) {
        //Check if input file entered
        if(strcmp(argument, "<") ==  0 ) {
            //Copy the next piece of input over to the inFile array/string
            argument = strtok(NULL, " ");
            strcpy(inFile, argument);
        }
        //Check if output file entered
        else if(strcmp(argument, ">") == 0) {
            //Copy next piece of input to outFile array/string
            argument =  strtok(NULL, " ");
            strcpy(outFile, argument);
        }
        //Check if input is an ampersand and implies a background process
        else if(strcmp(argument, "&") == 0) {
            *background = true;
        }
        //Any other input would be part of the command itself
        else {
            commandArr[i] = strdup(argument);

            //If $$ at end of command, replace it with pid
            for(int j = 0; commandArr[i][j]; j++) {
                if(commandArr[i][j] == '$' && commandArr[i][j+1] == '$') {
                    commandArr[i][j] = '\0';
                    snprintf(commandArr[i], CONSTSIZE, "%s%d", commandArr[i], processID);
                }
            }
        }
        //Reset argument to the next argument to continue loop
        argument = strtok(NULL, " ");
    }
}

//Function catches CTRL-Z command from keyboard and handles SIGTSTP signal
//Creating global variable for tracking whether background processes are allowed or not
//https://stackoverflow.com/questions/6970224/providing-passing-argument-to-signal-handler
void handleSIGTSTP(int signal) {
    
    //If background processes are currently possible, switch to that not being possible
    if(fgMode == false) {
        char* bkgMessage = "Entering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, bkgMessage, 50);
        fgMode = true;
    }

    else {
        char* bkgMessage2 = "Exiting foreground-only mode\n";
        write(STDOUT_FILENO, bkgMessage2, 30);
        fgMode = false;
    }
}

int main() {

    //Variable to control while loop of shell
    bool running = true;

    //Variable to hold the parsed command of the user, initialized to NULL
    char* command[MAXARGS];

    for(int i = 0; i < MAXARGS; i++) {
        command[i] = NULL;
    }

    //Variable to track whether user entered a background process or not
    bool bkgProc = false;

    //Variable to hold the exit status
    int status = 0;

    //Arrays to hold the name of input and output files entered by user
    char inputFile[CONSTSIZE] = "";
    char outputFile[CONSTSIZE] = "";

    //Get the process id for smallsh, the main shell
    int pid = getpid();

    //Signal for ctrl-C/SIGINT from Module 5 course notes, ensures that the shell doesn't
    //terminate and ignores the SIGINT signal

    //Initialize SIGINT_action struct
    struct sigaction SIGINT_action = {0};
    //Register SIG_IGN to be signal handler./
    SIGINT_action.sa_handler = SIG_IGN;
    //Block all catchable signals 
    sigfillset(&SIGINT_action.sa_mask);
    //No flags set
    SIGINT_action.sa_flags = 0;
    //Install signal handler
    sigaction(SIGINT, &SIGINT_action, NULL);

    //Signal for ctrl-Z/SIGTSTP redirects to handler handleSIGTSTP
    
    //Initialize SIGSTP_action struct
    struct sigaction SIGTSTP_action = {0};
    //Register handleSIGSTP signal handler
    SIGTSTP_action.sa_handler = handleSIGTSTP;
    //Block all catchable signals 
    sigfillset(&SIGTSTP_action.sa_mask);
    //Changed from no flags based on Piazza post
    SIGTSTP_action.sa_flags = SA_RESTART;
    //Install signal handler
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    //Manages the shell getting commands and running them until they exit
    while(running) {
        //Get command/input from user
        getCommand(command, &bkgProc, pid, inputFile, outputFile);

        //If the first character of input is a comment or blank, continue
        if(command[0][0] == '#' || command[0][0] == '\0') {
            continue;
        }
        //If user entered cd, change to directory entered
        else if(strcmp(command[0], "cd") == 0) {
            //Check if a directory was entered following cd command and change to directory
            if(command[1]) {
                if(chdir(command[1]) == -1) {
                    printf("Error! Directory not found.\n");
                    fflush(stdout);
                }
            }
            else {
                //If cd command called by itself, changes to directory specified in HOME environment 
                //https://www.tutorialspoint.com/c_standard_library/c_function_getenv.htm
                chdir(getenv("HOME"));
            }

        }
        //If user entered status
        else if(strcmp(command[0], "status") == 0) {
            //If exited by status, print exit value
            //https://www.geeksforgeeks.org/exit-status-child-process-linux/
            if(WIFEXITED(status)) {
                int exitStatus = WEXITSTATUS(status);
                printf("exit value %d\n", exitStatus);
                fflush(stdout);
            }
            //Terminated by signal
            else {
                int termStatus = WTERMSIG(status);
                printf("terminated by signal %d\n", termStatus);  
                fflush(stdout);
            }
        }
        //If user entered exit, exit shell... Pretty sure this is redundant, but all three are in
        //there and this thing is working like a well-oiled machine so it stays!
        else if(strcmp(command[0], "exit") == 0) {
            running = false;
            kill(0, SIGTERM);
            exit(0);
        }
        //User entered something besides our built-in commands, create new process
        else {
            //Spawn the child process
            pid_t childPid = -5;
            int source, target, result;

            //A majority of code in this section from course materials Module 5: Process I/O
            childPid = fork();

            switch(childPid) {
                case -1: 
                    perror("fork() failed\n");
                    exit(1);
                    break;

                //Child process
                case 0:
                    //From a child process ctrl-C/SIGINT behaves as usual
                    SIGINT_action.sa_handler = SIG_DFL;
                    sigaction(SIGINT, &SIGINT_action, NULL);

                    //Open input file and handle input
                    if(strcmp(inputFile, "")) {
                        source = open(inputFile, O_RDONLY);

                        if(source == -1) {
                            perror("Error ");
                            exit(1);
                        }
                        //Redirect stdin to input file
                        result = dup2(source, 0);

                        //Close source
                        fcntl(source, F_SETFD, FD_CLOEXEC);
                    }

                    //Open output file and handle output 
                    if(strcmp(outputFile, "")) {
                        target = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

                        if(target == -1) {
                            perror("cannot open file for output\n");
                            exit(1);
                        }
                        //Redirect stdout to target file
                        result = dup2(target, 1);

                        //Close target
                        fcntl(target, F_SETFD, FD_CLOEXEC);
                    }

                    if(execvp(command[0], command) == -1) {
                        printf("%s: no such file or directory\n", command[0]);
                        fflush(stdout);
                        exit(1);
                    }
                    break;
                
                default:
                    //Execute background process if fgmode is false and bkgProc called
                    if(bkgProc == true && fgMode == false) {
                        //Block parent until child terminates
                        pid_t currPid = waitpid(childPid, &status, WNOHANG);
                        printf("background pid is %d\n", childPid);
                        fflush(stdout);
                    }
                    //Wait for the child
                    else {
                        pid_t currPid = waitpid(childPid, &status, 0);
                    } 
                   
                while((childPid = waitpid(-1, &status, WNOHANG)) > 0) {
                    //If exited by status, print exit value
                    //https://www.geeksforgeeks.org/exit-status-child-process-linux/
                    if(WIFEXITED(status) != 0) {
                        printf("background pid %d is done: exit value %d\n", childPid, WEXITSTATUS(status));
                        fflush(stdout);
                    }
                    //Terminated by signal
                    else {
                        printf("background pid %d is done: terminated by signal %d\n", childPid, WTERMSIG(status));  
                        fflush(stdout);
                    }
                }
            }
            
        }    

        //Reset command array, input/output files, and background tracker for new input    
        for(int i = 0; command[i]; i++) {
            command[i] = NULL;
        }

        bkgProc = false;
        inputFile[0] = '\0';
        outputFile[0] = '\0';

    }

    return 0;
}
