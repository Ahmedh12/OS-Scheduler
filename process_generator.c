#include "headers.h"

/*Global Variables
*/
int ArrivalQueueID; //queue id at which the arrived processes will be sent to the schedular

int cid; //Clock ID
int sid; //Scheduler ID

void clearResources(int);

//Function to get process Count from Processes.txt file
int CountProcesses(char *fileName);

//Alarm Signal Handler Function
void handleAlarm(int SIGNUM);

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);
    /*Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    */

    //printf("please choose the desired scheduling Algoritm \n1.HPF\n2.SRTN\3.Round Robinsn\n");

    /*------------------------------------------------------------------------------------------------------------------------------------------
        Variables Decleration
    */
    const unsigned COLUMNS = 4;               //Number of fields for each process Entry
    unsigned ROWS = 0;                        //Number of processes available.
    struct generator_shedular_msg newProcess; //message structure for the message buffer used for communication between proc_gen and scheduler
    unsigned sentProcessesCount = 0;          //Number of processes sent to the schedular
    /*------------------------------------------------------------------------------------------------------------------------------------------
        Getting Process Count from Processes.txt
    */
    ROWS = CountProcesses("Processes.txt");
    /*------------------------------------------------------------------------------------------------------------------------------------------
        decleration of the process table to be populated with the data from  the processes.txt file 
    */
    int processTable[ROWS][COLUMNS];

    /*------------------------------------------------------------------------------------------------------------------------------------------
        Reading process data from txt file and populating the Data structure , 
        The Data structure is a 2d Array structured as follows
        each col constitutes a process entry where each entry is composed of 
        process_id,arrival_time,Burst_time,priority respectively
    */
    FILE *inputFile;                      //file handler to read from the process file.
    const unsigned MAX_BUFFER_SIZE = 128; //maximum buffer size to hold data read from process file.
    char buffer[MAX_BUFFER_SIZE];         //characters array , of buffer size, to hold the data read from the proceses file.
    int count = 0;                        //track of the number of lines read from the input file
    inputFile = fopen("processes.txt", "r");
    if (!inputFile)
    {
        printf("Failed Opening processes file");
        exit(-1);
    }
    fgets(buffer, MAX_BUFFER_SIZE, inputFile); //to ignore the headings of the processes.txt file.
    while (fgets(buffer, MAX_BUFFER_SIZE, inputFile))
    {

        int id, arrival, burst, priority;
        sscanf(buffer, "%d %d %d %d", &id, &arrival, &burst, &priority); // parse each number alone form the read line
        processTable[count][0] = id;
        processTable[count][1] = arrival;
        processTable[count][2] = burst;
        processTable[count][3] = priority;

        count++;
    }
    fclose(inputFile);
    /*------------------------------------------------------------------------------------------------------------------------------------------
        creating the queue where the schedular will recieve the arrived processes on 
    */
    ArrivalQueueID = msgget(ARRIVAL_QUEUE_ID, 0666 | IPC_CREAT);

    /*Initiate and create the scheduler and clock processes.
    */
    cid = fork();
    if (cid == 0)
        execl("./bin/clk.out", "clk.out", NULL);
    else
    {
        sid = fork();
        if (sid == 0)
            execl("./bin/scheduler.out", "scheduler.out", argv[1], argv[2], NULL);
    }

    /* Use this function after creating the clock process to initialize clock
    */
    initClk();

    /* attach the alarm signal handler to the alarm signal
    */
    signal(SIGALRM, handleAlarm);

    /*Send the information to the scheduler at the appropriate time.
    */
    while (sentProcessesCount < ROWS)
    {
        alarm(processTable[sentProcessesCount][1] - getClk());
        pause();
        while (processTable[sentProcessesCount][1] == getClk())
        {
            newProcess.mtag = NEW_PROCESS_TAG;
            newProcess.process_id = processTable[sentProcessesCount][0];
            newProcess.arrival_time = processTable[sentProcessesCount][1];
            newProcess.burst_time = processTable[sentProcessesCount][2];
            newProcess.priority = processTable[sentProcessesCount][3];
            msgsnd(ArrivalQueueID, &newProcess, sizeof(newProcess) - sizeof(newProcess.mtag), !IPC_NOWAIT);
            sentProcessesCount++;

            if (atoi(argv[1]) == 3) //case RR
            {
                //kill(sid, SIGCONT); //Continues the scheduler (if blocked) to update the process Table
                //kill(sid, SIGUSR1); //Wakes up the schedular to update the Process Table
            }

            else if (atoi(argv[1]) == 1) //case HPF
            {
                kill(sid, SIGCONT);
                kill(sid, SIGUSR1);
            }
            else if (atoi(argv[1]) == 2) //case SRTN
                kill(sid, SIGCONT);
        }
    }
    if (atoi(argv[1]) == 2)
        kill(sid, SIGUSR1); //Signals the scheduler per final Process

    /*Termination flag to scheduler
    */
    newProcess.mtag = NEW_PROCESS_TAG;
    newProcess.process_id = -1;
    msgsnd(ArrivalQueueID, &newProcess, sizeof(newProcess) - sizeof(newProcess.mtag), !IPC_NOWAIT);

    /*
    waiting for the schedular to finish
    */
    int status;
    int pid = wait(&status);
    /*Clear clock resources
    */
    printf("Schedular exited successfully....\nEnding Simulation\n");
    destroyClk(true);
}

void clearResources(int signum)
{
    msgctl(ArrivalQueueID, IPC_RMID, (void *)0); //delete the arrival queue
    //TODO Clears all resources in case of interruption
}

int CountProcesses(char *fileName)
{
    FILE *inputFile;                      //file handler to read from the process file.
    const unsigned MAX_BUFFER_SIZE = 128; //maximum buffer size to hold data read from process file.
    char buffer[MAX_BUFFER_SIZE];         //characters array , of buffer size, to hold the data read from the proceses file.
    inputFile = fopen("processes.txt", "r");
    int count = 0;
    if (!inputFile)
    {
        printf("Failed Opening processes file");
        exit(-1);
    }
    fgets(buffer, MAX_BUFFER_SIZE, inputFile); //to ignore the headings of the processes.txt file.
    while (fgets(buffer, MAX_BUFFER_SIZE, inputFile))
    {
        count++;
    }
    fclose(inputFile);
    return count;
}

void handleAlarm(int SIGNUM)
{
    //printf("process Arrival , Wake up Process Generator\n");
    signal(SIGALRM, handleAlarm);
}
