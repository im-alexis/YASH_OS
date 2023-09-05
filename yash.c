#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

typedef struct Command
{
    char *parsed_cmd[15];
    char *rdirect_filename;
    int pipe_flg;    // 0 no pipe, 1 pipe
    int rdirect_flg; // 0 (<) STDIN, 1 (>) STDOUT, 2 (2>) STDERR, -1 NO Modification
    int redirect_token_index;
    int redirect_fd;
} Command;
typedef struct Job
{
    int pid;    // unique identifier for the job 0 to Max
    int ground; // like background or foreground
    int status; // 0 stopped or 1 running
    Command cmd;
} Job;

void handle_signal(int signal)
{
    if (signal == SIGQUIT)
    {
        printf("Leaving");
        exit(0);
    }
    else if (signal == SIGINT)
    {
        printf("\nCtrl-c was pressed");
    }
    else if (signal == SIGTSTP)
    {
        printf("\nCtrl-z was pressed");
    }
}

int main()
{
    char *command;

    int fg_available = 1;
    int place_holder = 1; // this is a place holder for a terminal_kill_signal

    signal(SIGINT, &handle_signal);  // ctrl-c
    signal(SIGTSTP, &handle_signal); // ctrl-z
    // signal(SIGCHLD, &handle_signal);
    // signal(SIGCONT, &handle_signal);
    signal(SIGQUIT, &handle_signal); // ctrl-d

    while (place_holder) // this should loop while there isn't kill signal to the shell ()
    {
        if (fg_available)
        {
            command = readline("# ");
            if (strlen(command) > 0)
            {
                add_history(command);
            }

            parse_command(command);
        }
    }

    return 0;
}

void parse_command(char *raw_cmd)
{
    Command command_space[2] = {
        {.parsed_cmd = NULL, .rdirect_filename = NULL, .pipe_flg = 0, .rdirect_flg = -1, .redirect_fd = -1, .redirect_token_index = -1},
        {.parsed_cmd = NULL, .rdirect_filename = NULL, .pipe_flg = 0, .rdirect_flg = -1, .redirect_fd = -1, .redirect_token_index = -1}}; // parsed_cmd,filename, pipe_flg, rdirect_flg, rdirect_index_token, rdirect_fd

    char *token, *the_rest;
    the_rest = raw_cmd;

    int parse_tracker = 0;
    int cmd_space_tracker = 0;

    int pipe_flg = -1;
    int overall_rdirect_flg = -1;
    int invalid_cmd_flg = 0;

    while ((token = strtok_r(the_rest, " ", &the_rest)))
    {
        int its_not_r_or_p = notFileRedirectOrPipe(token);

        if (its_not_r_or_p && parse_tracker != command_space[cmd_space_tracker].redirect_token_index) // check to see if no pipe symbol and no file redirection
        {
            command_space[cmd_space_tracker].parsed_cmd[parse_tracker] = token;
        }
        else
        {
            if (command_space[cmd_space_tracker].redirect_token_index == -1 && strcmp(token, "|"))
            {
                command_space[cmd_space_tracker].rdirect_filename = strtok_r(the_rest, " ", &the_rest);
                overall_rdirect_flg = 1;
                if (!strcmp(token, "<"))
                {
                    command_space[cmd_space_tracker].rdirect_flg = 0;
                }
                else if (!strcmp(token, ">"))
                {
                    command_space[cmd_space_tracker].rdirect_flg = 1;
                }
                else if (!strcmp(token, "2>"))
                {
                    command_space[cmd_space_tracker].rdirect_flg = 2;
                }
            }
            else if (!strcmp(token, "|"))
            {
                parse_tracker = -1;
                pipe_flg = 1;
                command_space[cmd_space_tracker].pipe_flg = 1;
                cmd_space_tracker++;
            }
        }

        parse_tracker++;
    }

    if (pipe_flg == 1) // if theres is a pipe
    {
        int file_d[2];
        if (pipe(file_d) == -1)
        {
            return;
        }
        int pid1 = fork();
        if (pid1 < 0)
        {
            return;
        }
        if (pid1 == 0)
        {
            // This is the left side of the command
            dup2(file_d[1], STDOUT_FILENO);
            close(file_d[0]);
            close(file_d[1]);

            // execute_cmd(command_space, overall_rdirect_flg, 0);

            if (overall_rdirect_flg != -1)
            {
                fileRedirection(command_space, 0);
            }
            if (execvp(command_space[0].parsed_cmd[0], command_space[0].parsed_cmd) == -1)
            {
                perror("exec");
            }
        }
        int pid2 = fork();
        if (pid2 < 0)
        {
            return;
        }
        if (pid2 == 0)
        {
            // This is the right side of the command
            dup2(file_d[0], STDIN_FILENO);
            close(file_d[0]);
            close(file_d[1]);

            // execute_cmd(command_space, overall_rdirect_flg, 1);

            if (overall_rdirect_flg != -1)
            {
                fileRedirection(command_space, 1);
            }
            if (execvp(command_space[1].parsed_cmd[0], command_space[1].parsed_cmd) == -1)
            {
                perror("exec");
            }
        }
        /*Proper Closing of File Descriptors*/
        close(file_d[0]);
        close(file_d[1]);
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
    }
    else // run command as normal
    {
        int pid = fork();

        if (pid == 0)
        {
            // execute_cmd(command_space, overall_rdirect_flg, 0);
            if (overall_rdirect_flg != -1)
            {
                fileRedirection(command_space, 0);
            }

            // child process
            if (execvp(command_space[0].parsed_cmd[0], command_space[0].parsed_cmd) == -1)
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

void execute_cmd(Command cs[], int ord, int index)
{
    if (ord != -1)
    {
        fileRedirection(cs, index);
    }

    if (execvp(cs[0].parsed_cmd[0], cs[0].parsed_cmd) == -1)
    {
        perror("exec");
    }
    exit(0);
}

void fileRedirection(Command cmds[], int index)
{
    int descriptor = cmds[index].redirect_fd;
    char *file = cmds[index].rdirect_filename;
    int flg = cmds[index].rdirect_flg;
    if (flg == 1)
    {
        descriptor = open(file, O_CREAT | O_TRUNC | O_WRONLY, 0600);
        dup2(descriptor, STDOUT_FILENO);
        close(descriptor);
    }
    if (flg == 0)
    {
        descriptor = open(file, O_RDONLY);
        if (descriptor != -1)
        {
            dup2(descriptor, STDIN_FILENO);
            close(descriptor);
        }
        else
        {
            printf("The file does not exist");
        }
    }
    if (flg == 2)
    {
        descriptor = open(file, O_CREAT | O_TRUNC | O_WRONLY);
        dup2(descriptor, STDERR_FILENO);
        close(descriptor);
        printf("The file does not exist");
    }
}
