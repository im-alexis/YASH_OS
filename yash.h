#ifndef YASH_H
#define YASH_H

typedef struct Command
{
    char *parsed_cmd[15];
    char *STDIN_FILE;
    char *STDOUT_FILE;
    char *STDERR_FILE;
    int rdirect_flgs; // // b[0] (<) STDIN, b[1] (>) STDOUT, b[2] (2>) STDERR
    int pipe_flg;     // 0 no pipe, 1 pipe
    int bg_flg;
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
void handle_signal(int signal);

void parse_command(char *raw_cmd, Job *jobs);

int special_token_checker(char *token);

void execvp_call(Command cs[], int ord, int index);

int file_redirection(Command cmds[], int index);

void fg_cmd(Job *jobs);

void bg_cmd(Job *jobs);

void jobs_cmd(Job *jobs);

void clean_stack(int mode, Job *jobs);

void move_to_end(int index, Job *jobs);

void execute_cmd(int flgs, Command cmds[], char *raw_cmd, Job *Jobs);

int fg_bg_jobs_flgs(int flgs);

int index_of_pid(int id, Job *jobs);

#endif