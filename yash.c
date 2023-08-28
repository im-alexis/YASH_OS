#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>
#include <unistd.h>
#include <linux/limits.h>

struct job
{
    int id;     // unique identifier for the job 0 to Max
    int ground; // like background or foreground
    int status; // 0 stopped or 1 running
    char command[];
};

unsigned char *command;
char *token;

int main()
{
    while (1)
    {
        command = readline("# ");
        if (strlen(command) > 0)
        {
            add_history(command);
        }
        if (!strcmp(command, "ls"))
        {
            ls_func();
        }
        if (!strcmp(command, "pwd"))
        {
            pwd_func();
        }
    }

    return 0;
}

int echo_func()
{

    return 0;
}

int jobs_func()
{

    return 0;
}

void ls_func()
{
    DIR *directory;
    struct dirent *dir;
    directory = opendir(".");

    if (directory)
    {
        while ((dir = readdir(directory)) != NULL)
        {
            if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0 && dir->d_name[0] != '.') // skip over [. , .. , hidden files (start with .)]
            {
                printf("%s ", dir->d_name);
            }
        }
        printf("\n");
        closedir(directory);
    }
}
void pwd_func()
{
    char working_directory[PATH_MAX];
    getcwd(working_directory, sizeof(working_directory));
    printf("%s\n ", working_directory);
}
int cat_func()
{

    return 0;
}