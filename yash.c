#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <fcntl.h>

struct Job
{
    int pid;    // unique identifier for the job 0 to Max
    int ground; // like background or foreground
    int status; // 0 stopped or 1 running
    char *command;
};
typedef struct Job Job;

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
    char *token, *the_rest, *redirect_file;
    the_rest = cmd;
    int rdirect_flg = 4; // 0 (<) from another file, 1 (>) to another file , 2 (2>) erro, 3 (|) piping, 4 no redirect
    int i = 0;
    int redirect_token_index = -1;
    int redirect_fd;

    while ((token = strtok_r(the_rest, " ", &the_rest)))
    {
        if (notFileRedirectOrPipe(token) && i != redirect_token_index) // check to see if no pipe symbol and no file redirection
        {
            cmd_arr[i] = token;
        }
        else
        {
            if (!strcmp(token, "<") && redirect_token_index == -1)
            {
                rdirect_flg = 0;
                redirect_token_index = i + 1;
            }
            else if (!strcmp(token, ">") && redirect_token_index == -1)
            {
                rdirect_flg = 1;
                redirect_token_index = i + 1;
            }
            else if (!strcmp(token, "2>") && redirect_token_index == -1)
            {
                rdirect_flg = 2;
                redirect_token_index = i + 1;
            }
            else if (redirect_token_index == i && notFileRedirectOrPipe(token))
            {
                redirect_file = token;
            }
        }

        i++;
    }
    if (isValidExecCmd(cmd_arr[0]))
    {
        __pid_t pid = fork();

        if (pid == 0)
        {
            if (rdirect_flg == 1)
            {
                redirect_fd = open(redirect_file, O_CREAT | O_TRUNC | O_WRONLY, 0600);
                dup2(redirect_fd, STDOUT_FILENO);
                close(redirect_fd);
            }
            if (rdirect_flg == 0)
            {
                redirect_fd = open(redirect_file, O_RDONLY);
                if (redirect_fd != -1)
                {
                    dup2(redirect_fd, STDIN_FILENO);
                    close(redirect_fd);
                }
                else
                {
                    printf("The file does not exist");
                }
            }
            if (rdirect_flg == 2)
            {
                redirect_fd = open(redirect_file, O_CREAT | O_TRUNC | O_WRONLY);

                dup2(redirect_fd, STDERR_FILENO);
                close(redirect_fd);
                printf("The file does not exist");
            }
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
    else
    {
    }
}

int isValidExecCmd(char *cmd_tok)
{
    if (!strcmp(cmd_tok, "ls") || !strcmp(cmd_tok, "cat") || !strcmp(cmd_tok, "echo") || !strcmp(cmd_tok, "time") || !strcmp(cmd_tok, "pwd") || !strcmp(cmd_tok, "grep") || !strcmp(cmd_tok, "sleep"))
    {
        return 1;
    }

    return 0;
}

int notFileRedirectOrPipe(char *token)
{
    if (strcmp(token, ">") && strcmp(token, "<") && strcmp(token, "2>") && strcmp(token, "|"))
    {
        return 1;
    }
    return 0;
}
