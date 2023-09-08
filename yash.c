#include <stdio.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>

typedef struct Command
{
    char *parsed_cmd[15];
    char *rdirect_filename;
    int pipe_flg;    // 0 no pipe, 1 pipe
    int rdirect_flg; // 0 (<) STDIN, 1 (>) STDOUT, 2 (2>) STDERR, -1 NO Modification
    int redirect_token_index;
    int redirect_fd;
    int bg_flg;
} Command;
typedef struct Job
{
    int pid; // the pid of the process
    int stack_id;
    int running; // 0 stopped or 1 running
    int pstatus;
    char *og_cmd;
} Job;

Job jobs_stack[20];
int table_population;
int history_value;

void handle_signal(int signal)
{
    switch (signal)
    {
    case SIGCHLD:
        break;
    case SIGQUIT:
        exit(0);
    case SIGINT:
        break;
    case SIGTSTP:
        break;
    }
    return;
}

int main()
{
    char *command;
    table_population = 0;
    history_value = 1;

    signal(SIGINT, handle_signal);  // ctrl-c
    signal(SIGTSTP, handle_signal); // ctrl-z
    signal(SIGCHLD, handle_signal);
    signal(SIGQUIT, handle_signal); // ctrl-d

    while (1) // this should loop while there isn't kill signal to the shell ()
    {
        command = readline("# ");
        if (command != EOF)
        {
            if (strlen(command) > 0)
            {
                add_history(command);
            }
            parse_command(command);
            // free(command);
        }
    }

    return 0;
}

void parse_command(char *raw_cmd)
{
    Command command_space[2] = {
        {.parsed_cmd = NULL, .rdirect_filename = NULL, .pipe_flg = 0, .rdirect_flg = -1, .redirect_fd = -1, .redirect_token_index = -1, .bg_flg = 0},
        {.parsed_cmd = NULL, .rdirect_filename = NULL, .pipe_flg = 0, .rdirect_flg = -1, .redirect_fd = -1, .redirect_token_index = -1, .bg_flg = 0}}; // parsed_cmd,filename, pipe_flg, rdirect_flg, rdirect_index_token, rdirect_fd

    char *token, *the_rest, *cpy;
    cpy = malloc(sizeof(raw_cmd));
    the_rest = cpy;
    strcpy(the_rest, raw_cmd);

    int parse_tracker = 0;
    int cmd_space_tracker = 0;

    // potentially swtich all flags into one number and go by bits
    int pipe_flg = 0;
    int overall_rdirect_flg = 0;
    int invalid_cmd_flg = 0;
    int background_flg = 0;
    int first_token_recieved = 0;
    int fg_cmd_flg = 0;
    int bg_cmd_flg = 0;
    int jobs_cmd_flg = 0;

    while ((token = strtok_r(NULL, " ", &the_rest)))
    {

        int its_r_p_b = special_token_checker(token);
        if (!strcmp(token, "fg"))
        {
            if (fg_cmd_flg || bg_cmd_flg || jobs_cmd_flg)
            {
                invalid_cmd_flg = 1;
                break;
            }
            if (first_token_recieved)
            {
                invalid_cmd_flg = 1;
                break;
            }
            fg_cmd_flg = 1;
            first_token_recieved = 1;
            continue;
        }
        if (!strcmp(token, "bg"))
        {
            if (fg_cmd_flg || bg_cmd_flg || jobs_cmd_flg)
            {
                invalid_cmd_flg = 1;
                break;
            }

            if (first_token_recieved)
            {
                invalid_cmd_flg = 1;
                break;
            }
            bg_cmd_flg = 1;
            first_token_recieved = 1;
            continue;
        }
        if (!strcmp(token, "jobs"))
        {
            if (fg_cmd_flg || bg_cmd_flg || jobs_cmd_flg)
            {
                invalid_cmd_flg = 1;
                break;
            }
            if (first_token_recieved)
            {
                invalid_cmd_flg = 1;
                break;
            }
            jobs_cmd_flg = 1;
            first_token_recieved = 1;
        }

        if (!its_r_p_b && parse_tracker != command_space[cmd_space_tracker].redirect_token_index) // check to see if token is not a special token
        {
            command_space[cmd_space_tracker].parsed_cmd[parse_tracker] = token;
            first_token_recieved = 1;
        }
        else if (!first_token_recieved && its_r_p_b) // if there has not been a valid first token and you recieve (>, <, 2>, &) cmd is auto invalid
        {
            invalid_cmd_flg = 1;
            break;
        }
        else
        {
            if (command_space[cmd_space_tracker].redirect_token_index == -1 && strcmp(token, "|") && strcmp(token, "&"))
            {
                if (first_token_recieved)
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
            }
            else if (!strcmp(token, "|"))
            {
                if (background_flg) // if you piped and have a & then thats an error
                {
                    invalid_cmd_flg = 1;
                    break;
                }
                parse_tracker = -1;
                pipe_flg = 1;
                command_space[cmd_space_tracker].pipe_flg = 1;
                cmd_space_tracker++;
            }
            else if (!strcmp(token, "&"))
            {
                background_flg = 1;
                command_space[cmd_space_tracker].bg_flg = 1;
            }
        }

        parse_tracker++;
        // free(token);
    }
    if (!invalid_cmd_flg && (fg_cmd_flg != 1 && bg_cmd_flg != 1 && jobs_cmd_flg != 1)) // checks if the invalid command flag was triggered
    {
        if (pipe_flg) // if theres is a pipe
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
            if (!pid1)
            {
                // This is the left side of the command
                dup2(file_d[1], STDOUT_FILENO);
                close(file_d[0]);
                close(file_d[1]);
                execute_cmd(command_space, overall_rdirect_flg, 0);
            }

            int pid2 = fork();
            if (pid2 < 0)
            {
                return;
            }
            if (!pid2)
            {
                // This is the right side of the command
                dup2(file_d[0], STDIN_FILENO);
                close(file_d[0]);
                close(file_d[1]);

                execute_cmd(command_space, overall_rdirect_flg, 1);
            }

            /*Proper Closing of File Descriptors*/
            close(file_d[0]);
            close(file_d[1]);
            Job process = {.pid = pid2, .og_cmd = raw_cmd, .stack_id = history_value, .running = 1, .pstatus = 0};
            waitpid(pid1, NULL, WUNTRACED);
            if (background_flg && table_population < 20)
            {

                jobs_stack[table_population] = process;
                table_population++;
                history_value++;
                waitpid(pid2, &process.pstatus, WNOHANG);
            }
            else
            {
                waitpid(pid2, &process.pstatus, WUNTRACED);
                if (WIFSTOPPED(process.pstatus))
                {
                    jobs_stack[table_population] = process;
                    table_population++;
                    history_value++;
                }
            }
        }
        else // run command as normal
        {
            int pid = fork(); // this pid is the child
            if (!pid)
            {
                execute_cmd(command_space, overall_rdirect_flg, 0);
                exit(0);
            }
            else
            {
                Job process = {.pid = pid, .og_cmd = raw_cmd, .stack_id = history_value, .running = 1, .pstatus = 0};
                if (background_flg && table_population < 20)
                {
                    jobs_stack[table_population] = process;
                    table_population++;
                    history_value++;

                    waitpid(pid, &process.pstatus, WNOHANG);
                }
                else
                {
                    waitpid(pid, &process.pstatus, WUNTRACED);
                    if (WIFSTOPPED(process.pstatus))
                    {
                        process.running = 0;
                        jobs_stack[table_population] = process;
                        table_population++;
                        history_value++;
                    }
                }
            }
        }
    }
    if (fg_cmd_flg)
    {
        fg_cmd();
    }
    else if (bg_cmd_flg)
    {
        bg_cmd();
    }
    else if (jobs_cmd_flg)
    {
        jobs_cmd();
    }
    free(cpy);
}

int isValidExecCmd(char *cmd_tok)
{
    if (!strcmp(cmd_tok, "ls") || !strcmp(cmd_tok, "cat") || !strcmp(cmd_tok, "echo") || !strcmp(cmd_tok, "time") || !strcmp(cmd_tok, "pwd") || !strcmp(cmd_tok, "grep") || !strcmp(cmd_tok, "sleep"))
    {
        return 1;
    }

    return 0;
}

int special_token_checker(char *token)
{
    if (strcmp(token, ">") && strcmp(token, "<") && strcmp(token, "2>") && strcmp(token, "|") && strcmp(token, "&"))
    {
        return 0;
    }
    return 1;
}

void execute_cmd(Command cs[], int ord, int index)
{
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    if (ord != -1)
    {
        file_redirection(cs, index);
    }
    if (execvp(cs[index].parsed_cmd[0], cs[index].parsed_cmd) == -1)
    {
        perror("exec");
    }
    exit(0);
}

void file_redirection(Command cmds[], int index)
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
            printf("The file does not exist\n");
        }
    }
    if (flg == 2)
    {
        descriptor = open(file, O_CREAT | O_TRUNC | O_WRONLY);
        dup2(descriptor, STDERR_FILENO);
        close(descriptor);
        printf("The file does not exist\n");
    }
}

void fg_cmd()
{
    Job top_stack = jobs_stack[table_population - 1];
    int ts_pid = top_stack.pid;

    kill(ts_pid, SIGCONT);
    // printf("Continuing pid: %d", ts_pid);
    waitpid(ts_pid, &top_stack.pstatus, WUNTRACED);
    if (WIFEXITED(top_stack.pstatus))
    {
        printf("[%d] + Done    %s\n", top_stack.stack_id, top_stack.og_cmd);
        table_population--;
    }
    return;
}

void bg_cmd()
{
}

void jobs_cmd()
{
    for (int i = 0; i < table_population; i++)
    {
        if (i == (table_population - 1))
        {
            if (jobs_stack[i].running)
            {
                printf("[%d] + Running    %s\n", jobs_stack[i].stack_id, jobs_stack[i].og_cmd);
            }
            else
            {
                printf("[%d] + Stopped    %s\n", jobs_stack[i].stack_id, jobs_stack[i].og_cmd);
            }
        }
        else
        {
            if (jobs_stack[i].running)
            {
                printf("[%d] - Running    %s\n", jobs_stack[i].stack_id, jobs_stack[i].og_cmd);
            }
            else
            {
                printf("[%d] - Stopped    %s\n", jobs_stack[i].stack_id, jobs_stack[i].og_cmd);
            }
        }
    }
}

void clean_stack()
{
}
