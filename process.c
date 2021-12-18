#include "headers.h"
#include <time.h>

/* Modify this file as needed*/
int remainingtime;
int startedAt;  //The time of start since last block
int timeWorked; //The time worked since last continued
int totalTimeWorked;
int totalBurstTime;

int main(int agrc, char *argv[])
{
    initClk();
    int start = getClk();
    remainingtime = atoi(argv[2]); //-----setting the remaining time
    int Finish_time = 0;

    if (atoi(argv[1]) == 2)
    {
        while (remainingtime > 0)
        {
            if (start + 1 <= getClk())
            {
                remainingtime--;
                start = getClk();
            }
        }
    }
    else
    {
        while (remainingtime > 0)
        {
            if (start + 1 == getClk())
            {
                remainingtime--;
                start = getClk();
            }
        }
    }

    destroyClk(false);

    if (atoi(argv[1]) == 2)
    {
        kill(getppid(), SIGCONT);
        kill(getppid(), SIGUSR2);
    }
    else if (atoi(argv[1]) == 3)
    {
        kill(getppid(), SIGUSR1);
    }
    else
    {
        exit(0);
    }
}
