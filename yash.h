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

void parse_command(char *cmd);

int isValidExecCmd(char *cmd_tok);

int special_token_checker(char *token);

void execute_cmd(Command cs[], int ord, int index);

void file_redirection(Command cmds[], int index);

void handle_signal(int signal);
