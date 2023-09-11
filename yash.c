#include <stdio.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdlib.h>
// #include "yash.h"

#define pipe_flg_msk 0x0001
#define overall_rdirect_flg_msk 0x0002
#define invalid_cmd_flg_msk 0x0004
#define background_flg_msk 0x0008
#define first_token_recieved_msk 0x0010
#define fg_cmd_flg_msk 0x0020
#define bg_cmd_flg_msk 0x0040
#define jobs_cmd_flg_msk 0x0080

typedef struct FileRedirection
{
    char *STDIN_FILE;
    char *STDOUT_FILE;
    char *STDERR_FILE;
    int rdirect_flgs; // b[0] (<) STDIN, b[1] (>) STDOUT, b[2] (2>) STDERR
    int redirect_token_index;
    int redirect_fd;
} FileRedirection;
typedef struct Command
{
    char *parsed_cmd[15];
    char *rdirect_filename;
    int pipe_flg;    // 0 no pipe, 1 pipe
    int rdirect_flg; // 0 (<) STDIN, 1 (>) STDOUT, 2 (2>) STDERR, -1 NO Modification
    int redirect_token_index;
    int redirect_fd;
    int bg_flg;
    FileRedirection files;
} Command;
typedef struct Job
{
    int pid; // the pid of the process
    int stack_id;
    int running; // 0 stopped or 1 running
    int pstatus;
    char *og_cmd;
    int bg_flg;
} Job;

int table_population;
int history_value;
int recent_stop;

void handle_signal(int signal)
{
    switch (signal)
    {
    case SIGCHLD:
        // Candidate for stack clean up location
        break;
    case SIGQUIT:
        exit(0);
    case SIGINT:
        break;
    case SIGTSTP:
        break;
    case SIGTTOU:
        break;
    }
    return;
}

int main()
{
    char *command;
    table_population = 0;
    history_value = 1;
    recent_stop = -1;
    Job jobs_stack[20];
    // jobs_stack = malloc(sizeof(Job) * 20);

    signal(SIGINT, handle_signal);  // ctrl-c
    signal(SIGTSTP, handle_signal); // ctrl-z
    signal(SIGCHLD, handle_signal);
    // signal(SIGQUIT, handle_signal); // ctrl-d
    signal(SIGTTOU, handle_signal);

    while (1)
    {
        clean_stack(0, jobs_stack); // periodic call to stack clean up
        command = readline("# ");
        if (!command)
        {
            break;
        }
        if (strlen(command) > 0)
            add_history(command);
        parse_command(command, jobs_stack);
    }

    printf("\n");
    return 0;
}

void parse_command(char *raw_cmd, Job *jobs)
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

    int flgs = 0; // pipe_flg | overall_rdirect_flg | invalid_cmd_flg | background_flg | first_token_recieved | fg_cmd_flg | bg_cmd_flg | jobs_cmd_flg

    while ((token = strtok_r(NULL, " ", &the_rest)))
    {
        int its_r_p_b = special_token_checker(token);

        if (!strcmp(token, "fg") || !strcmp(token, "bg") || !strcmp(token, "jobs"))
        {
            int ret = fg_bg_jobs_flgs(flgs);
            if (ret)
            {
                flgs = flgs | invalid_cmd_flg_msk;
                break;
            }
            if (!strcmp(token, "fg"))
                flgs = flgs | fg_cmd_flg_msk | first_token_recieved_msk;
            if (!strcmp(token, "bg"))
                flgs = flgs | bg_cmd_flg_msk | first_token_recieved_msk;
            if (!strcmp(token, "jobs"))
                flgs = flgs | jobs_cmd_flg_msk | first_token_recieved_msk;

            continue;
        }
        if (!its_r_p_b && parse_tracker != command_space[cmd_space_tracker].redirect_token_index) // check to see if token is not a special token
        {
            command_space[cmd_space_tracker].parsed_cmd[parse_tracker] = token;
            flgs = flgs | first_token_recieved_msk;
        }
        else if (!(first_token_recieved_msk | flgs) && its_r_p_b) // if there has not been a valid first token and you recieve (>, <, 2>, &) cmd is auto invalid
        {
            flgs = flgs | invalid_cmd_flg_msk;
            break;
        }
        else
        {
            if (command_space[cmd_space_tracker].redirect_token_index == -1 && strcmp(token, "|") && strcmp(token, "&"))
            {
                if ((first_token_recieved_msk | flgs))
                {
                    command_space[cmd_space_tracker].rdirect_filename = strtok_r(the_rest, " ", &the_rest);
                    flgs = flgs | overall_rdirect_flg_msk;

                    if (!strcmp(token, "<"))
                        command_space[cmd_space_tracker].rdirect_flg = 0;
                    else if (!strcmp(token, ">"))
                        command_space[cmd_space_tracker].rdirect_flg = 1;
                    else if (!strcmp(token, "2>"))
                        command_space[cmd_space_tracker].rdirect_flg = 2;
                }
            }
            else if (!strcmp(token, "|"))
            {
                if ((background_flg_msk | flgs)) // if you piped and have a & then thats an error
                {
                    flgs = flgs | invalid_cmd_flg_msk;
                    break;
                }
                parse_tracker = -1;
                flgs = flgs | pipe_flg_msk;
                command_space[cmd_space_tracker].pipe_flg = 1;
                cmd_space_tracker++;
            }
            else if (!strcmp(token, "&"))
            {
                flgs = flgs | background_flg_msk;
                command_space[cmd_space_tracker].bg_flg = 1;
            }
        }
        parse_tracker++;
    }
    execute_cmd(flgs, command_space, raw_cmd, jobs);
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

void execvp_call(Command cs[], int ord, int index)
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

void fg_cmd(Job *jobs)
{
    if (table_population > 0)
    {
        Job top_stack = jobs[table_population - 1];
        int ts_pid = top_stack.pid;

        kill(ts_pid, SIGCONT);
        int val = waitpid(ts_pid, &top_stack.pstatus, WUNTRACED);
        if (WIFEXITED(top_stack.pstatus))
        {
            printf("[%d] + Done    %s\n", top_stack.stack_id, top_stack.og_cmd);
            table_population--;
        }
    }
    return;
}

void bg_cmd(Job *jobs)
{
    if (recent_stop != -1)
    {
        int rs_index = index_of_pid(recent_stop, jobs);
        if (rs_index != -1)
        {
            Job process = jobs[rs_index];
            jobs[rs_index].running = 1;
            printf("[%d] + Running    %s \n", process.stack_id, process.og_cmd);
            // printf("PID: %d | Running Status: %d | Stack ID: %d\n", process.pid, process.running, process.stack_id);
            kill(process.pid, SIGCONT);
            // printf("Sent %d a SIGCONT\n", process.pid);
            //  waitpid(process.pid, &process.pstatus, WNOHANG);
            move_to_end(rs_index, jobs);
        }
    }
}

void jobs_cmd(Job *jobs)
{

    clean_stack(1, jobs);
    for (int i = 0; i < table_population; i++)
    {
        Job process = jobs[i];
        // printf("PID: %d | Running Status: %d | Stack ID: %d\n", process.pid, process.running, process.stack_id);
        if (i == (table_population - 1))
        {
            if (jobs[i].running == 1)
                printf("[%d] + Running    %s\n", jobs[i].stack_id, jobs[i].og_cmd);
            else
                printf("[%d] + Stopped    %s\n", jobs[i].stack_id, jobs[i].og_cmd);
        }
        else
        {
            if (jobs[i].running == 1)
                printf("[%d] - Running    %s\n", jobs[i].stack_id, jobs[i].og_cmd);
            else
                printf("[%d] - Stopped    %s\n", jobs[i].stack_id, jobs[i].og_cmd);
        }
    }
}

void clean_stack(int mode, Job *Jobs)
{

    // Mode 0 for when clean_stack is called from main
    // Mode 1 for when clean stecks is called from jobs cmd

    Job tmp[20];
    int temp_index = 0;
    int rm_val = 0;
    // if (mode)
    //     printf("Called clean_stack(), from jobs\n");
    // else
    //     printf("Called clean_stack(), from main\n");
    // printf("Table Population: %d\n", table_population);
    for (int i = 0; i < table_population; i++)
    {

        Job process = Jobs[i];
        int val = waitpid(process.pid, &process.pstatus, WNOHANG);
        printf("Pid: %d | Background Flag: %d | FOR LOOP INDEX: %d | waitpid returned: %d \n", process.pid, process.bg_flg, i, process.pstatus);
        // if (WIFEXITED(process.pstatus))
        // {
        //     printf("Child process exited with status %d\n", WEXITSTATUS(process.pstatus));
        // }
        // else
        // {
        //     printf("Child process terminated abnormally.\n");
        // }

        if (WIFEXITED(process.pstatus) && mode == 1 && process.bg_flg == 1)
        {
            printf("[%d] + Done    %s\n", process.stack_id, process.og_cmd);
            rm_val++;
        }
        else if (WIFEXITED(process.pstatus) && mode == 0 && process.bg_flg == 0)
        {
            printf("[%d] + Done    %s\n", process.stack_id, process.og_cmd);
            rm_val++;
        }
        else
        {
            tmp[temp_index] = Jobs[i];
            temp_index++;
        }
    }
    for (int i = 0; i < table_population; i++)
    {
        Jobs[i] = tmp[i];
    }
    table_population = table_population - rm_val;
    if (table_population == 0)
    {
        history_value = 1;
    }
}

void move_to_end(int index, Job *jobs)
{
    if (index != (table_population - 1))
    {
        Job tmp[20];
        for (int i = 0; i < table_population; i++)
        {
            if (index != i)
            {
                tmp[i] = jobs[i];
            }
        }
        tmp[table_population - 1] = jobs[index];
        for (int i = 0; i < table_population; i++)
        {
            jobs[i] = tmp[i];
        }
    }
}

void execute_cmd(int flgs, Command cmds[], char *raw_cmd, Job *Jobs)
{
    if (!(flgs & invalid_cmd_flg_msk) && !(flgs & fg_cmd_flg_msk) && !(flgs & bg_cmd_flg_msk) && !(flgs & jobs_cmd_flg_msk) && table_population < 20) // checks if the invalid command flag was triggered and if there is space
    {
        if (flgs & pipe_flg_msk) // if theres is a pipe
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
                execvp_call(cmds, (overall_rdirect_flg_msk & flgs), 0);
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

                execvp_call(cmds, (overall_rdirect_flg_msk & flgs), 1);
            }

            /*Proper Closing of File Descriptors*/
            close(file_d[0]);
            close(file_d[1]);
            Job process = {.pid = pid2, .og_cmd = raw_cmd, .stack_id = history_value, .running = 1, .pstatus = 0, .bg_flg = 0};
            waitpid(pid1, NULL, WUNTRACED);
            if ((flgs & background_flg_msk))
            {

                Jobs[table_population] = process;
                table_population++;
                history_value++;
                process.bg_flg = 1;
                waitpid(pid2, &process.pstatus, WNOHANG);
            }
            else
            {
                waitpid(pid2, &process.pstatus, WUNTRACED);
                if (WIFSTOPPED(process.pstatus))
                {
                    Jobs[table_population] = process;
                    recent_stop = process.stack_id;
                    process.running == 0;
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
                execvp_call(cmds, (overall_rdirect_flg_msk & flgs), 0);
                exit(0);
            }
            else
            {
                Job process = {.pid = pid, .og_cmd = raw_cmd, .stack_id = history_value, .running = 1, .pstatus = 0, .bg_flg = 0};
                if ((flgs & background_flg_msk) && table_population < 20)
                {
                    process.bg_flg = 1;
                    Jobs[table_population] = process;
                    table_population++;
                    history_value++;
                    process.bg_flg = 1;
                    waitpid(pid, &process.pstatus, WNOHANG);
                }
                else
                {
                    waitpid(pid, &process.pstatus, WUNTRACED);
                    if (WIFSTOPPED(process.pstatus))
                    {
                        process.running = 0;
                        Jobs[table_population] = process;
                        recent_stop = process.stack_id;
                        table_population++;
                        history_value++;
                    }
                }
            }
        }
    }
    if (flgs & fg_cmd_flg_msk)
        fg_cmd(Jobs);
    else if (flgs & bg_cmd_flg_msk)
        bg_cmd(Jobs);
    else if (jobs_cmd_flg_msk & flgs)
        jobs_cmd(Jobs);
}

int fg_bg_jobs_flgs(int flgs)
{
    if ((flgs & fg_cmd_flg_msk) || (flgs & bg_cmd_flg_msk) || (flgs & jobs_cmd_flg_msk))
    {
        return 1;
    }
    if (flgs & first_token_recieved_msk)
    {
        return 1;
    }
    return 0;
}

int index_of_pid(int id, Job *jobs)
{
    for (int i = 0; i < table_population; i++)
    {
        if (id == jobs[i].stack_id)
        {
            return i;
        }
    }
    return -1;
}
