#include "shell.h"

//function descriptions are in smallsh.h
void cd(){

    //strings to hold the current directory and to point to the path of the input
    char curr_dir[MAX_LENGTH];
    char * dir_path;

    //get the current directory and then get the path from the input
    getcwd(curr_dir, sizeof curr_dir);
    dir_path = strstr(input, " ");

    //if there was a path from input
    if (dir_path){
        //increment the path
        dir_path++;
        //add a "/" to the end of current path
        strcat(curr_dir, "/");
        //concat the two
        strcat(curr_dir, dir_path);
        //switch to the new directory
        chdir(curr_dir);
    }
    else{
        //otherwise go home
        chdir(getenv("HOME"));
    }

    //get the current directory and print out where you are
    getcwd(curr_dir, sizeof curr_dir);
    printf("%s\n", curr_dir);
    fflush(stdout);
}

void status(){
    //print out the exit status code
    printf("exit value %d\n", EXT_CODE);
    fflush(stdout);
}

void new_proc(){
    
    //fork the process
    spawn_pid = fork();

    //if the fork is unsuccessful, prompt the user
    if (spawn_pid < 0){
        printf("\nCannot Fork\n");
        fflush(stdout);
    }
    //if it is successful
    else if (spawn_pid == 0){
        //if the input has kill
        if (TSTP_count > 0 && strstr(input, "kill") != NULL){
            
            //string to store the input to be killed
            char cmd[MAX_LENGTH] = {0};

            //length of the string without the signal
            int tmp = strlen(input) - 11;

            //copy over the input without the extra stuff
            strncpy(cmd, input, tmp);
            //copy over the cmd into input
            strcpy(input,cmd);

            //convert the pid into a string and put it in to the command
            sprintf(cmd, "%d", getpid());
            //put the command on top of input to then execute
            strcat(input, cmd);
        }
        //call exec_cmd to execute the command
        exec_cmd();
    }
    //otherwise
    else{
        //if spawned in the background
        if (B_FLAG){
            //add the pid to the list of backgroud procs
            b_procs[b_procs_count] = spawn_pid;
            b_procs_count++;
            //don't wait for the child proc
            waitpid(spawn_pid, &PROC_EXT_FLAG, WNOHANG);
            //set the b flag back
            B_FLAG = false;

            //print child process id 
            printf("\nbackground process id: %d \n", spawn_pid);
            fflush(stdout);
        }
        else{
            //wait for child process
            waitpid(spawn_pid, &PROC_EXT_FLAG, 0);
            
            //if exited, then throw error
            if(WIFEXITED(PROC_EXT_FLAG)){
                EXT_CODE = WEXITSTATUS(PROC_EXT_FLAG);
            }
        }
    }
    B_FLAG = false;
}

void catch_SIGINT(int signal){
    //prompt the user
    printf("\nProcess interrupted by %d\n", signal);
}

void catch_SIGTSTP(){

    //if the stop flag was not set
    if (!TSTP_FLAG){
        //set the count to 0 and the background flag to false
        TSTP_count = 0;
        B_FLAG = false;

        //set the flag
        TSTP_FLAG = true;

        //prompt user
        fflush(stdout);    
        printf("\nEntering foreground-only mode (& is now ignored)\n");
    
        //increment the count
        TSTP_count++;
    }
    //if the flag was set
    else{
        //unset the flag
        TSTP_FLAG = false;
        
        //prompt user
        fflush(stdout);
   		printf("\nExiting foreground-only mode\n");

        //increment the count
        TSTP_count++;
    }
}

void replace_string(char * src, char * substr, char * newstr){
    
    //get the substring from the source
    char * tmp = strstr(src, substr);
    
    //if the substring is not present, then exit
    if (tmp == NULL){
        return;
    }

    //replace the substring with the new substring and add it to the main string
    memmove(tmp + strlen(newstr), tmp + strlen(substr), strlen(tmp) - strlen(substr) + 1);
    memcpy(tmp, newstr, strlen(newstr));
}

void expand_cmd(){

    //string to hold the pid
    char arg_$$[MAX_LENGTH] = {0};
    //get the pid
    sprintf(arg_$$, "%d", getppid());
    
    //temp string to hold the input to traverse it
    char * tmp = input;
    int i = 0;

    //count the instances of $$ in input
    while((tmp = strstr(tmp, "$$")) != NULL){
        i++;
        tmp+=2;
    }

    //for all the instances, replace the substring
    for(int j = 0; j < i; j++){
        replace_string(input, "$$", arg_$$);
    }

}

void get_background_procs(){
    
    //traverse the list of procs in the background
    for (int i=0; i < b_procs_count; i++){
        
        //don't let the parent proc be blocked and waiting by checking on background procs
        if((spawn_pid = waitpid(b_procs[i], &PROC_EXT_FLAG, WNOHANG)) > 0){
            //if signaled, prompt user
            if (WIFSIGNALED(PROC_EXT_FLAG)){
                printf("\nbackground pid %d is done: terminated by signal %d\n", b_procs[i], WTERMSIG(PROC_EXT_FLAG));
            }
            //if exited, prompt user
            if (WIFEXITED(PROC_EXT_FLAG)){
                //prompt user
                printf("\nexit value: %d\n", WEXITSTATUS(PROC_EXT_FLAG));
            }
        }
    }
}

void set_background_proc(){
    
    //string to hold command
    char cmd[MAX_LENGTH] = {0};

    //take off the & and the space
    int tmp = (strlen(input) - 2);

    //if we are not in foreground mode
    if(!TSTP_FLAG){
        B_FLAG = true;
    }

    //put the command back in to input, without the space and &
    strncpy(cmd, input, tmp);
    strcpy(input, cmd);

}

void kill_background_procs(){
    //traverse the list of background procs and terminate them
    for (int i=0; i < b_procs_count; i++){
        kill(b_procs[i], 2);
    }
}

void get_cmd(){
    //if input is exit, set the quit flag
    if (strncmp(input, "exit", 4) == 0){
        Q_FLAG = false;
    }
    //if status, call status
    else if (strncmp(input, "status", 6) == 0){
        status();
    }
    //if cd, call cd()
    else if (strncmp(input, "cd", 2) == 0){
        cd();
    }
    //if a space or a comment, ignore
    else if (strcmp(input, " ") == 0 || strncmp(input, "#", 1) == 0){
        //do nothing 
    }
    //otherwise call new_proc to work with the command
    else{
        new_proc();
    }
}

void get_input(){
    
    //print out the : for the user
    printf(": ");
    fflush(stdout);

    //get the user input
    fgets(input, sizeof input, stdin);

    //get rid of the newline 
    input[strcspn(input, "\n")] = '\0';

    //if the $$ variable is found, expand
    if (strstr(input, "$$") != NULL){
        expand_cmd();
    }

    //if the & is found, run in background
    if (strchr(input,'&') != NULL){
        set_background_proc();
    }
}

void exec_cmd(){

    //iterator and file descriptor
    int i = 0;
    int fd;

    //set the first command to the first word in input
    cmds[0] = strtok(input, " ");

    //put all the words from input into the array
    while(cmds[i] != NULL){
        i++;
        cmds[i] = strtok(NULL, " ");
    }

    //set i back to 0 to reuse it
    i=0;

    //traverse the cmds array
    while (cmds[i] != NULL){
        //if > is found
        if (strcmp(cmds[i], ">") == 0){
            //open the next argument as a file, create or read
            fd = open(cmds[i+1], O_CREAT | O_WRONLY, 0777);
            //if file could not be opened or created
            if (fd < 0){
                //prompt the user 
                printf("\nUnable to open or create %s\n", cmds[i+1]);
                //exit
                fflush(stdout);
                exit(1);
            }
            else{
                //call dup2 with stdout
                dup2(fd, STDOUT_FILENO);
                //set the redirect command to nothing
                cmds[i] = 0;
                //execute the command 
                execvp(cmds[0], cmds);
                //close the file
                close(fd);
            }
        }
        //if the other rd is found
        else if (strcmp(cmds[i], "<") == 0){
            
            //open the file as read
            fd = open(cmds[i+1], O_RDONLY, 0);

            //if the file could not be opened
            if (fd < 0){
                //prompt the user 
                printf("\nUnable to open %s\n", cmds[i+1]);

                //exit
                fflush(stdout);
                exit(1);
            }
            else{

                //call dup2 with stdin
                dup2(fd, STDIN_FILENO);
                //set the rd to nothing
                cmds[i] = 0;
                //execute
                execvp(cmds[0], cmds);
                //close
                close(fd);
            }
        }

        //increment i for traversing cmds
        i++;
    }

    //if the command could not be found, prompt the user
    if ((EXT_CODE = execvp(cmds[0], cmds)) != 0){
        printf("\n%s: no such file or directory\n", input);
        exit(EXT_CODE);
    }
}

int main(){
    
    //set the signals up for stop and interrupt
    //so that they are handled by the program and not by the default shell
    struct sigaction interrupt = {0};
    struct sigaction stop = {0};

    memset(&interrupt, 0, sizeof interrupt);
    memset(&stop, 0, sizeof stop);

    interrupt.sa_handler = catch_SIGINT;
    sigfillset(&interrupt.sa_mask);
    sigaction(SIGINT, &interrupt, NULL);

    stop.sa_handler = catch_SIGTSTP;
    sigfillset(&stop.sa_mask);
    sigaction(SIGTSTP, &stop, NULL);
    
    //while exit has not been called
    while(Q_FLAG){
        get_background_procs();
        get_input();
        get_cmd();
    }

    //kill all remaining background procs upon exiting
    kill_background_procs();
    
    return 0;
}

