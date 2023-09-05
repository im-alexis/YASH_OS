typedef struct Command
{
    char *parsed_cmd;
    char *rdirect_filename;
    int pipe_flg;
    int rdirect_flg;
    int redirect_token_index;
    int redirect_fd;
} Command;

typedef struct Job
{
    int pid;
    int ground;
    int status;
    Command cmd;
} Job;

void process_command(char *cmd);

int isValidExecCmd(char *cmd_tok);

int notFileRedirectOrPipe(char *token);

void fileRedirection(Command cmds[], int index);

void execute_command();