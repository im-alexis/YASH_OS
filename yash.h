typedef struct Command
{
    char *parsed_cmd;
    char *rdirect_filename;
    int pipe_flg;
    int rdirect_flg;
    int redirect_token_index;
    int redirect_fd;
    int bg_flg;
} Command;

typedef struct Job
{
    int pid;
    int stack_id;
    int running;
    int pstatus;
    char *og_cmd;
} Job;

void parse_command(char *cmd);

int isValidExecCmd(char *cmd_tok);

int special_token_checker(char *token);

void execvp_call(Command cs[], int ord, int index);

void file_redirection(Command cmds[], int index);

void handle_signal(int signal);

void fg_cmd();

void bg_cmd();

void jobs_cmd();

void clean_stack();

void execute_cmd(int flgs, Command cmds[], char *raw_cmd);

int fg_bg_jobs_flgs(int flgs);
