#include <stdio.h>
#include <stdlib.h>

struct job
{
    int id;     // unique identifier for the job 0 to Max
    int ground; // like background or foreground
    int status; // 0 stopped or 1 running
    char command[];
};

int main()
{
    while (1)
    {
        printf("# ");
        char word[20];
        scanf("%s", word);
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

int ls_func()
{

    return 0;
}
int pwd_func()
{

    return 0;
}
int cat_func()
{

    return 0;
}