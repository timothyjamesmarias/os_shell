#ifndef SMALLSH_H
#define SMALLSH_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

//macros for the length of an input
#define MAX_LENGTH 2048
#define MAX_ARGS 512
#define MAX_PROCS 50

//string to hold the user input
char input[MAX_LENGTH];

//array of commands from input
char * cmds[MAX_ARGS];

//flag to signal exiting the program
bool Q_FLAG = true;

//id for new process
pid_t spawn_pid = -1;

//flag for if TSTP is caught
bool TSTP_FLAG = false;
//count for TSTP
int TSTP_count = 0;

//flag for if a process is running in the backgorund
bool B_FLAG = false;
//array of processes running in the background
pid_t b_procs[MAX_PROCS];
//the count for how many procs are running in the background
int b_procs_count = 0;
//flag for exiting processes
int PROC_EXT_FLAG = -1;

//exit code for when a process exits
uint EXT_CODE;

/*
cd function
-gets current directory
-changes directory from arguments in input
-prints current directory to stdout
-gets called when first argument is "cd" in input
*/
void cd();

/*
status function
-gets the exit value of the last run process
-prints exit value to stdout
-gets called when "status" is a command in input
*/
void status();

/*
new_proc function
-forks a new process
-executes command by calling exec_cmd
-if the background flag is set, adds process to the list of background procs
*/
void new_proc();

/*
catch_SIGINT fuction
-if the interrupt signal is caught, prints message to stdout
*/
void catch_SIGINT();

/*
catch_SIGTSTP function
-if the stop signal is caught, it will change the stop flag
-toggles between foreground and background mode
-prompts the user upon changing modes
*/
void catch_SIGTSTP();

/*
replace_string(3) function
-helper for expand_cmd
-finds and instance of a substring within a string and replaces it with a new string
-alters src
-src: the original string
-substr: the string we are looking for
-newstr: the new string to replace the substring
I used this tutorial https://www.youtube.com/watch?v=0qSU0nxIZiE to help me create this function
*/
void replace_string(char * src, char * substr, char * newstr);

/*
expand_cmd function
-finds the number of instances where "$$" is in input
-calls replace_string to replace those instances with the smallsh pid
*/
void expand_cmd();

/*
get_background_procs function
-goes through list of background processes in b_procs
-prints to stdout if a proc in the backgroud was exited or terminated
*/
void get_background_procs();

/*
set_background_proc function
-called when input has the '&' to signal a background process
-sets the B_FLAG to true
*/
void set_background_proc();

/*
kill_background_procs function
-called at the end of the program when the user inputs "exit"
-takes the list of background procs and kills them
*/
void kill_background_procs();

/*
get_cmd function
-handles the built in procs of cd, status, exit, and comments
-calls new_proc if those are not found
*/
void get_cmd();

/*
get_input function
-gets user input from stdin and puts it into the input string
-calls expand_cmd or set_background_proc if "$$" or "&" is found in the input
*/
void get_input();

/*
exec_cmd function
-takes in the command from cmds
-if input has a redirection, calls dup2 to handle it
-calls execvp to execute the command in cmds[0]
*/
void exec_cmd();

#endif