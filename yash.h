
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

void parse_command(char *raw_cmd);

int isValidExecCmd(char *cmd_tok);

int special_token_checker(char *token);

void execvp_call(Command cs[], int ord, int index);

void file_redirection(Command cmds[], int index);

void handle_signal(int signal);

void fg_cmd();

void bg_cmd();

void jobs_cmd();

void clean_stack(int mode);

void execute_cmd(int flgs, Command cmds[], char *raw_cmd);

void move_to_end(int index);

int fg_bg_jobs_flgs(int flgs);
