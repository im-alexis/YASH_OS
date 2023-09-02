#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>
#include <unistd.h>
#include <linux/limits.h>

struct job
{
    int id;     // unique identifier for the job 0 to Max
    int ground; // like background or foreground
    int status; // 0 stopped or 1 running
    char command[];
};

int main()
{
    char *command;

    int fg_available = 1;
    int place_holder = 1; // this is a place holder for a terminal_kill_signal

    while (place_holder) // this should loop while there isn't kill signal to the shell ()
    {
        if (fg_available)
        {
            command = readline("# ");
            if (strlen(command) > 0)
            {
                add_history(command);
            }

            process_command(command);
        }
    }

    return 0;
}

void process_command(char *cmd)
{
    char *cmd_arr[10] = {NULL};
    char *token, *the_rest;
    the_rest = cmd;
    int i = 0;
    while ((token = strtok_r(the_rest, " ", &the_rest)))
    {
        cmd_arr[i] = token;
        i++;
    }
    if (isValidCMD(cmd_arr[0]))
    {
        __pid_t pid = fork();

        if (pid == 0)
        {
            // child process
            if (execvp(cmd_arr[0], cmd_arr) == -1)
            {
                perror("exec");
            }
            exit(0);
        }
        if (pid > 0)
        {
            if (wait(0) == -1)
            {
                perror("wait");
            }
        }
    }
}

int isValidCMD(char *cmd_tok)
{
    if (!strcmp(cmd_tok, "ls") || !strcmp(cmd_tok, "cat") || !strcmp(cmd_tok, "echo") || !strcmp(cmd_tok, "time") || !strcmp(cmd_tok, "pwd") || !strcmp(cmd_tok, "jobs") || !strcmp(cmd_tok, "grep"))
    {
        return 1;
    }

    return 0;
}

void echo_func()
{

    return 0;
}
void time_func()
{

    return 0;
}

int jobs_func()
{

    return 0;
}

void ls_func(int mod) // 0 - no modifier | 1 - (-a) | 2 - (-l)
{
    DIR *directory;
    struct dirent *dir;
    directory = opendir(".");

    if (directory)
    {
        while ((dir = readdir(directory)) != NULL)
        {

            if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0 && dir->d_name[0] != '.' && mod == 0) // skip over [. , .. , hidden files (start with .)]
            {
                printf("%s ", dir->d_name);
            }
        }
        printf("\n");
        closedir(directory);
    }
}
void pwd_func()
{
    char working_directory[PATH_MAX];                     // creates a buffer for the string to go into
    getcwd(working_directory, sizeof(working_directory)); // fills the buffer with the current working directotry
    printf("%s\n ", working_directory);                   // outputs to the console
}
int cat_func()
{

    return 0;
}