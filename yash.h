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

int notFileRedirectOrPipe(char *token);

void execute_cmd(Command cs[], int ord, int index);

void fileRedirection(Command cmds[], int index);

void handle_signal(int signal);

void handle_SIGINT(int signal);

void handle_SIGTSTP(int signal);

void handle_SIGCHLD(int signal);

void handle_SIGCONT(int signal);

void handle_SIGQUIT(int signal);