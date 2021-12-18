#include "headers.h"

int ArrivalQueueID;
int ProcessQueueIDout;
int RemainingTimeMemory;                  //ID of shared memory
struct generator_shedular_msg newProcess; //message structure to recieve data about newly arrived processes from processes generator

//==================================================Khaled============================================================================
//SRTN Algorithm Variables
struct Queue *SRTNQ;
int PCB_processes_count = 0;
int runningProcessPID;
int receivedProcesses;
int startedAt; //Current Running Process started (resumed) at
bool shorterProcess;
bool messagesEnded;
bool processesFinished;
struct process_msg msg; //Constructing a message with burstTime

// used by RR too
double accumulativeWTA;       // total WeightedTurnarount Time
int accumulativeWT;           // total Waiting Time
int accumulativeBT;           // total Burst Time
float accumulativeWTAsquared; // total WeightedTurnarount Time squared

//SRTN Algorithm Functions
void handlerSRTNCONT(int sigNum);
void handlerSRTNUSR1(int sigNum); //When final process is received
void handlerSRTNUSR2(int sigNum); //When a process ends
void putAndResume(struct generator_shedular_msg *newProcess);
//====================================================================================================================================

//=======================================================Millania=====================================================================
//Round Robins-specific  Variables
struct Queue *RRQ;
struct Node *runningProcess;
int quantum;
int alarmVal;
char *charAlgo;
char *roundRobin_output_filename = "scheduler.log";
char *roundRobin_statistics_filename = "scheduler.perf";
int finishedProcesses = 0;

//Round Robins-specific Functions
void handlerRRUSR1(int sigNum);     //Child finishes
void handlerRRSigAlarm(int sigNum); //When a quantum is over
void receive_process();
void append_log_file(int currentTime, int id, char *state, int arrival, int burst, int remaining, int waiting, int TA, float WTA);
//====================================================================================================================================

//=================================================Tawheed============================================================================

struct generator_shedular_msg newProcess; //message structure to recieve data about newly arrived processes from processes generator

int status = 0;
int index_HPF = 0;           //index: indicates the index each of process entered the scheduler
int remaining_processes = 0; // I need this to prevent forking incase i already forked all sent processes
bool running = false;        //indication if process is running
Node_HPF *pq_HPF;            //Priority queue
PCB_Current Current_process;
int ArrivalQueueID = -1; //queue id at which the arrived processes will be sent to the scheduler
float count_HPF = 0;     //count: indicates the number of elements in the priority queue
float TOTAL_WTA = 0;     //WTA OF EACH PROCESS
float WTA = 0;
float Total_Waiting_Time = 0; //sum of waiting time
float TOTAL_BURST_HPF = 0;    //Getting total Burst time of all processes to calculate utilization
FILE *fp;                     //A FILE VARIABLE TO MAKE A .LOG FILE
float TOTAL_WTA_squared;
bool endProcesses = false;
void handlerHPFUSR1(int sigNum);
float TA;

int main(int argc, char *argv[])
{
    initClk();
    /*
    getting the queue Key for which the arrived processes will be sent on by the process scheduler
    */
    ArrivalQueueID = -1; //queue id at which the arrived processes will be sent to the scheduler
    while (ArrivalQueueID == -1)
    {
        ArrivalQueueID = msgget(ARRIVAL_QUEUE_ID, !IPC_CREAT);

        if (ArrivalQueueID == -1)
        {
            printf("Process Generator did not create the queue yet\n");
        }
    }

    charAlgo = argv[1];
    //Main Loop
    switch (atoi(argv[1])) //Enters the loop for chosen algorithm
    {
    case 1:
    {
        //=========================================== FORMING THE Scheduler.log ======================================//
        fp = fopen("scheduler.log", "w");
        if (fp == NULL)
        {
            printf("Error in file creation\n");
        }
        else
        {
            fprintf(fp, "#At \t time \t x \t process\t y \t state \t arr \t w \t total \t z \t remain \t y \t wait \t k\n");
        }
        fclose(fp);
        //==============================================================================================================//

        //Defining signal handler
        signal(SIGUSR1, handlerHPFUSR1);
        //Main Loop
        while (true)
        {
            //Ahmed Tawheed
            //HPF Implementation
            //================================= Initiate and create the received process[FORKING] ========================================//
            if (remaining_processes == 0)
            {
                // THIS STOP WILL ONLY BE AWAKENED WHEN THERE IS ATLEAST ONE PROCESS IN PCB
                // WILL ONLY BE AWAKENED BY THE SIGNAL SENT FROM THE PROCESS GENERATOR [SIGUSR1]
                // BUT WILL NOT BE AWAKENED BY THE PRCOCESS
                raise(SIGSTOP);
            }

            if (remaining_processes > 0)
            {
                //RUNNIGNG VARIABLE: TO INDICATE THE RUNNING OF A PROCESS
                running = true;

                //FORKING THE PROCESSS
                int cid = fork(); //0 --> child(process), 1 --> scheduler
                Current_process.Arrival_time = pq_HPF->Arrival_time;
                Current_process.Burst_time = pq_HPF->Burst_time;
                Current_process.end_time = pq_HPF->end_time;
                Current_process.pid = pq_HPF->pid;
                Current_process.priority = pq_HPF->priority;
                Current_process.Process_id_read = pq_HPF->Process_id_read;
                Current_process.start_time = pq_HPF->start_time;
                Current_process.status = pq_HPF->status;
                Current_process.waiting_time = pq_HPF->waiting_time;

                //printf("%d\n",Current_process->Burst_time);
                char burstTime_process[7];

                sprintf(burstTime_process, "%d", pq_HPF->Burst_time);

                if (cid == 0)
                {
                    //child[process]
                    execl("./bin/process.out", "process.out", charAlgo, burstTime_process, NULL);
                }
                else
                {
                    //parent[scheduler]
                    //pq_HPF is the head of the priority queue will contian the node of the highest priority
                    //I poped the current process from the priority queue, so i dont run it again
                    //as if i removed it in the end it's order would change while its execuiting and the time comes to pop
                    //a process i would actually be poping another process that came while running the curren process
                    Dequeue_HPF(&pq_HPF);
                    remaining_processes--;

                    Current_process.pid = cid; //SET THE PROCESS SYSTEM ID

                    //CALCULATING PROCESS VARIABLES
                    //-----setting the Process status
                    Current_process.status = 0;                                                               //set status as running
                    Current_process.start_time = getClk();                                                    //set the start process time
                    Current_process.waiting_time = Current_process.start_time - Current_process.Arrival_time; //time process waited before start running

                    //UPDATE THE LOG FILE
                    fp = fopen("scheduler.log", "a+");
                    if (fp == NULL)
                    {
                        printf("Error in file creation\n");
                    }
                    else
                    {
                        fprintf(fp, "At\t time\t %d\t process\t %d\t started\t arr\t %d\t total\t %d\t remain\t %d\t wait\t %d\t\n", getClk(), Current_process.Process_id_read, Current_process.Arrival_time, Current_process.Burst_time, Current_process.Burst_time, Current_process.waiting_time);
                    }
                    fclose(fp);

                    // STOP THE SCHEDULER PROCESS UNTIL THE PROCESS FINISHES RUNNING
                    //printf("=================================SCHEDULER.OUT IS SLEEPING===========================================\n");
                    int cpid = wait(&status); //THE SCHEDULER WILL SLEEP WAITING FOR THE PROCESS TO FINISH

                    //SETTING THE END TIME OF THE PROCESS
                    Current_process.end_time = getClk();
                    Current_process.status = 2;

                    //======================== Enter the data of the process in output file ==================//
                    //CALULATION STATS OF EACH VARIABLE: TA, WTA
                    //Calculate TA
                    TA = Current_process.end_time - Current_process.Arrival_time;

                    //Calculate WTA
                    //WTA OF EACH PROCESS LIST
                    //WTA: FOR EACH PROCESS
                    //TOTAL_WTA: FOR ALL PROCESSES
                    float WTA;
                    if (Current_process.Burst_time == 0)
                    {
                        WTA = 0;
                    }
                    else
                    {
                        WTA = TA / Current_process.Burst_time;
                    }

                    //1:WTA CALCULATION
                    TOTAL_WTA = TOTAL_WTA + WTA;
                    //CALCULATE THE WTA_SQUARED FOR WHEN GETTING STD
                    TOTAL_WTA_squared = TOTAL_WTA_squared + (WTA * WTA);

                    //TOTAL WAITING TIME OF PROCESS
                    Total_Waiting_Time = Total_Waiting_Time + Current_process.waiting_time;

                    //TOTAL BURST TIME FOR CPU UTILIZATION
                    TOTAL_BURST_HPF = TOTAL_BURST_HPF + Current_process.Burst_time;

                    //UPDATE THE SCHEDULER.LOG
                    fp = fopen("scheduler.log", "a+");
                    if (fp == NULL)
                    {
                        printf("Error in file creation\n");
                    }
                    else
                    {
                        fprintf(fp, "At\t time\t %d\t process\t %d\t finished\t arr\t %d\t total\t %d\t remain\t %d\t wait\t %d\t TA\t %.0f \t WTA \t %.2f\n", getClk(), Current_process.Process_id_read, Current_process.Arrival_time, Current_process.Burst_time, 0, Current_process.waiting_time, TA, WTA);
                    }
                    fclose(fp);

                    //SETTING RUNNING TO FALSE WHEN A PROCESS IS NOT RUNNING
                    running = false;
                }
            }

            //WHEN I RECEIVE ALL PROCESSES AND NO PROCESS IS REMAINING
            // I WILL END SIMULATION
            if (endProcesses && remaining_processes == 0)
            {
                break;
            }
        }
        //Delete the Queue1 object
        //Delete the Queue2 object
        //THE FINAL PROCESS INDIDCATOR

        //CALCULATION OF AVG WTA, AVG WAITING AND STD WTA
        float AVG_TOTAL_WTA = TOTAL_WTA / count_HPF;
        float AVG_WAIT_VAL = Total_Waiting_Time / count_HPF;
        float variance = (TOTAL_WTA_squared / count_HPF) - ((TOTAL_WTA / count_HPF) * (TOTAL_WTA / count_HPF));

        //CALCULATE STD
        float temp = variance / 2;
        float STD_TOTAL_WTA = (variance / temp + temp) / 2;
        int utilization = 100 * TOTAL_BURST_HPF / getClk();

        //UPDATE THE SCHEDULER.PERF FILE WITH FINAL STATS OF ALGORITHM
        fp = fopen("scheduler.perf", "w");
        if (fp == NULL)
        {
            printf("Error in file creation\n");
        }
        else
        {
            fprintf(fp, "CPU Utilization = %d%%\nAvg_WTA = %.2f\nAvg_Waiting : %.2f\nstd WTA : %.2f\n", utilization, AVG_TOTAL_WTA, AVG_WAIT_VAL, STD_TOTAL_WTA);
        }
        fclose(fp);

        destroyClk(true);
        printf("Ended processing...\n");
        exit(0);

        break;
    }
    case 2:
    {
        fp = fopen("scheduler.log", "w"); //("c:\\temp\\logfile.txt")
        if (fp == NULL)
        {
            printf("Error in file creation\n");
        }
        else
        {
            fprintf(fp, "#At \t time \t x \t process\t y \t state \t arr \t w \t total \t z \t remain \t y \t wait \t k\n");
        }
        fclose(fp);

        if (SRTNQ == NULL)
            SRTNQ = construct_queue(); //Creates new Queue for SRTN

        signal(SIGCONT, handlerSRTNCONT);
        signal(SIGUSR1, handlerSRTNUSR1);
        signal(SIGUSR2, handlerSRTNUSR2);

        PCB_processes_count = 0;
        runningProcessPID = -1;
        shorterProcess = false;
        receivedProcesses = 0;

        messagesEnded = false;
        processesFinished = false;

        accumulativeWTA = 0;
        accumulativeWTAsquared = 0;
        accumulativeWT = 0;
        accumulativeBT = 0;

        while (true)
        {
            if (runningProcessPID == -1) //No process is running now
            {
                if (PCB_processes_count == 0)
                {
                    if (messagesEnded && processesFinished)
                    {
                        char c = '%';
                        float std = sqrt((double)accumulativeWTAsquared / receivedProcesses - (double)(accumulativeWTA / receivedProcesses) * (accumulativeWTA / receivedProcesses));
                        FILE *write = fopen("scheduler.perf", "w");
                        fprintf(write, "CPU utilization = %.2f%c\nAvg WTA = %.2f\nAvg Waiting = %.2f\nStd WTA = %.2f\n",
                                100 * ((float)accumulativeBT / (getClk() - 1)), c, (float)accumulativeWTA / receivedProcesses,
                                (float)accumulativeWT / receivedProcesses, std);
                        fclose(write);
                        destroyClk(false);
                        exit(0);
                    }
                }
                if (SRTNQ->head)
                {
                    runningProcessPID = SRTNQ->head->pid;
                    kill(runningProcessPID, SIGCONT);
                    startedAt = getClk();
                    int currentTime = getClk(),
                        arr = SRTNQ->head->arrival_time, burstTime = SRTNQ->head->burst_time,
                        rem = SRTNQ->head->remaining_time;
                    if (SRTNQ->head->started)
                    {
                        FILE *write = fopen("scheduler.log", "a");
                        fprintf(write, "At time %d process %d resumed arr %d total %d remain %d wait %d\n",
                                currentTime, SRTNQ->head->process_id, arr, burstTime - rem, rem,
                                currentTime - arr - rem + burstTime);
                        fclose(write);
                    }
                    else
                    {
                        FILE *write = fopen("scheduler.log", "a");
                        fprintf(write, "At time %d process %d started arr %d total %d remain %d wait %d\n",
                                currentTime, SRTNQ->head->process_id, arr, burstTime - rem, rem,
                                currentTime - arr - rem + burstTime);
                        SRTNQ->head->started = true;
                        fclose(write);
                    }
                }
            }
            else if (shorterProcess) //A shorter process is found
            {
                int currentTime = getClk(),
                    arr = SRTNQ->head->arrival_time, burstTime = SRTNQ->head->burst_time,
                    rem = SRTNQ->head->remaining_time;

                if (SRTNQ->head)
                {
                    FILE *write = fopen("scheduler.log", "a");
                    fprintf(write, "At time %d process %d stopped arr %d total %d remain %d wait %d\n",
                            currentTime, SRTNQ->head->process_id, arr, burstTime - rem, rem,
                            currentTime - arr - rem + burstTime);
                    fclose(write);
                }

                if (runningProcessPID != -1)
                    kill(runningProcessPID, SIGSTOP);

                runningProcessPID = SRTNQ->head->pid;
                kill(runningProcessPID, SIGCONT);
                startedAt = getClk();

                arr = SRTNQ->head->arrival_time, burstTime = SRTNQ->head->burst_time,
                rem = SRTNQ->head->remaining_time;

                if (SRTNQ->head->started)
                {
                    FILE *write = fopen("scheduler.log", "a");
                    fprintf(write, "At time %d process %d resumed arr %d total %d remain %d wait %d\n",
                            currentTime, SRTNQ->head->process_id, arr, burstTime - rem, rem,
                            currentTime - arr - rem + burstTime);
                    fclose(write);
                }

                else
                {
                    FILE *write = fopen("scheduler.log", "a");
                    fprintf(write, "At time %d process %d started arr %d total %d remain %d wait %d\n",
                            currentTime, SRTNQ->head->process_id, arr, burstTime - rem, rem,
                            currentTime - arr - rem + burstTime);
                    SRTNQ->head->started = true;
                    fclose(write);
                }
                shorterProcess = false;
            }
            pause();
        }
        break;
    }
    case 3:
    {
        FILE *file = fopen(roundRobin_output_filename, "w");
        if (file == NULL)
            printf("Error opening %s", roundRobin_output_filename);
        fprintf(file, "#At time \t x \t process \ty \t state \t arr \t w \t total \t z\t remain \t y \t wait \t k \n"); // print the first line in the output text
        fclose(file);

        quantum = atoi(argv[2]);
        if (RRQ == NULL)
            RRQ = construct_queue(); //constructing a queue data structure composed of a head and a tail pointers to hold the ready process for RR algorithm

        signal(SIGALRM, handlerRRSigAlarm);
        signal(SIGUSR1, handlerRRUSR1);
        signal(SIGCONT, SIG_DFL);

        runningProcess = NULL; //Node to hold the data of the currently running process
        messagesEnded = false; //flag to indicate that the last process has been sent by the scheduler
        while (true)
        {
            if (!messagesEnded)
                receive_process();
            if (RRQ->head || runningProcess != NULL) //If there are processes in the ready queue or a process is already running
            {
                if (runningProcess != NULL) // if a process is running
                {
                    kill(runningProcess->pid, SIGSTOP);

                    runningProcess->state = WAITING;
                    int value = runningProcess->remaining_time - quantum;
                    if (value > 0)
                        runningProcess->remaining_time = value;
                    else
                    {
                        runningProcess->remaining_time = 0;
                    }

                    if (runningProcess->remaining_time == 0)
                    {
                        raise(SIGUSR1);
                    }
                    else
                    {
                        struct Node *stoppedProcess = DeepCopy(runningProcess);
                        enqueue(RRQ, stoppedProcess);

                        append_log_file(getClk(),
                                        runningProcess->process_id,
                                        runningProcess->state,
                                        runningProcess->arrival_time,
                                        runningProcess->burst_time,
                                        runningProcess->remaining_time,
                                        runningProcess->waiting_time,
                                        0,
                                        0);
                    }
                }

                runningProcess = DeepCopy(RRQ->head);
                dequeue(RRQ);
                runningProcess->next = NULL;
                runningProcess->state = RUNNING;
                runningProcess->started = true;

                if (runningProcess->remaining_time - runningProcess->burst_time != 0)
                    runningProcess->state = "resumed";

                int time_worked_so_far = runningProcess->burst_time - runningProcess->remaining_time;        // how many cycles the process has executed till now
                runningProcess->waiting_time = getClk() - runningProcess->arrival_time - time_worked_so_far; // waiting time

                append_log_file(getClk(),
                                runningProcess->process_id,
                                runningProcess->state,
                                runningProcess->arrival_time,
                                runningProcess->burst_time,
                                runningProcess->remaining_time,
                                runningProcess->waiting_time,
                                0,
                                0);

                kill(runningProcess->pid, SIGCONT);
                if (quantum > runningProcess->remaining_time)
                {
                    alarm(runningProcess->remaining_time);
                }
                else
                {
                    alarm(quantum);
                }

                pause();
            }
            else if (!messagesEnded)
            {
                raise(SIGSTOP);
            }
        }

        break;
    }

    default:
        break;
    }
}

//=================================================================SRTN=============================================================================

void handlerSRTNCONT(int SIGNUM)
{
    struct msqid_ds buf;
    int rc = msgctl(ArrivalQueueID, IPC_STAT, &buf);
    int num_messages = buf.msg_qnum; //Gets the count of messages in the message Queue

    int time = 0;
    //time = getRemainingTime(); //Gets the remainingtime of the current running process

    if (SRTNQ->head && SRTNQ->head->pid == runningProcessPID)
    {
        time = SRTNQ->head->remaining_time - (getClk() - startedAt);
        SRTNQ->head->remaining_time = time;
    }

    while (num_messages > 0) //Put all received processes into the PCB
    {
        msgrcv(ArrivalQueueID, &newProcess, sizeof(newProcess) - sizeof(newProcess.mtag), NEW_PROCESS_TAG, IPC_NOWAIT);
        if (newProcess.process_id == -1)
        {
            break;
        }
        else
        {
            pid_t pid = fork(); //Creates new process into the SRTNQ
            if (pid == 0)
            {
                execl("./bin/process.out", "process.out", charAlgo, convertIntegerToChar(newProcess.burst_time), NULL);
            }
            else if (receivedProcesses < newProcess.process_id) //Stop the created process, enqueue it and send its burst time
            {
                kill(pid, SIGSTOP);
                if (EnqueuePriority(SRTNQ, newProcess.process_id, newProcess.arrival_time,
                                    newProcess.burst_time, newProcess.priority, pid))
                    shorterProcess = true;

                //printf("Process Received with arrival time %d, burst time %d and pid %d\n", newProcess.arrival_time, newProcess.burst_time, pid);
                PCB_processes_count++;
                receivedProcesses++; //To prevent duplicate processes received from process generator
            }
        }
        rc = msgctl(ArrivalQueueID, IPC_STAT, &buf);
        num_messages = buf.msg_qnum;
    }
    signal(SIGCONT, handlerSRTNCONT);
}

void handlerSRTNUSR1(int sigNum) //All Processes are sent
{
    msgrcv(ArrivalQueueID, &newProcess, sizeof(newProcess) - sizeof(newProcess.mtag), NEW_PROCESS_TAG, IPC_NOWAIT);
    messagesEnded = true;
    //printf("Processes Ended\n");
    signal(SIGUSR1, handlerSRTNUSR1);
}

void handlerSRTNUSR2(int sigNum) //Child finishes
{
    int currentTime = getClk(), arr = SRTNQ->head->arrival_time, burstTime = SRTNQ->head->burst_time;
    FILE *write = fopen("scheduler.log", "a");
    fprintf(write, "At time %d process %d finished arr %d total %d remain 0 wait %d TA %d WTA %.2f\n",
            currentTime, SRTNQ->head->process_id, arr, burstTime,
            currentTime - arr - burstTime, currentTime - arr, (float)(currentTime - arr) / burstTime);
    fclose(write);

    if (burstTime != 0)
    {
        accumulativeWTA = accumulativeWTA + ((float)(currentTime - arr) / burstTime);
        accumulativeWTAsquared = accumulativeWTAsquared + ((float)(currentTime - arr) / burstTime) * ((float)(currentTime - arr) / burstTime);
    }
    accumulativeWT = accumulativeWT + currentTime - arr - burstTime;
    accumulativeBT = accumulativeBT + burstTime;

    dequeue(SRTNQ);

    PCB_processes_count--;

    if (PCB_processes_count == 0) //In case of nothing to run
    {
        runningProcessPID = -1;
        if (messagesEnded) //In case of nothing to run and all processes have finished
            processesFinished = true;
        return;
    }
    runningProcessPID = SRTNQ->head->pid;
    kill(runningProcessPID, SIGCONT);
    startedAt = getClk();

    currentTime = getClk(), arr = SRTNQ->head->arrival_time, burstTime = SRTNQ->head->burst_time;
    int rem = SRTNQ->head->remaining_time;
    if (SRTNQ->head->started)
    {
        FILE *write = fopen("scheduler.log", "a");
        fprintf(write, "At time %d process %d resumed arr %d total %d remain %d wait %d\n",
                currentTime, SRTNQ->head->process_id, arr, burstTime - rem, rem,
                currentTime - arr - rem + burstTime);
        fclose(write);
    }
    else
    {
        FILE *write = fopen("scheduler.log", "a");
        fprintf(write, "At time %d process %d started arr %d total %d remain %d wait %d\n",
                currentTime, SRTNQ->head->process_id, arr, burstTime - rem, rem,
                currentTime - arr - rem + burstTime);
        SRTNQ->head->started = true;
        fclose(write);
    }
    signal(SIGUSR2, handlerSRTNUSR2);
}

//=============================================================Round Robins==========================================================================

void receive_process()
{
    if (msgrcv(ArrivalQueueID, &newProcess, sizeof(newProcess) - sizeof(newProcess.mtag), NEW_PROCESS_TAG, IPC_NOWAIT) != -1)
    {
        printf("a message received\n");
        if (newProcess.process_id == -1)
        {
            messagesEnded = true;
            return;
        }
        else
        {
            pid_t pid = fork(); //Creates new process into the RRQ
            if (pid == 0)
            {
                execl("./bin/process.out", "process.out", charAlgo, convertIntegerToChar(newProcess.burst_time), NULL);
            }
            else //Stop the created process, enqueue it and send its burst time
            {
                kill(pid, SIGSTOP);
                struct Node *node = construct_node(newProcess.process_id, newProcess.arrival_time, newProcess.burst_time, newProcess.priority, pid);
                enqueue(RRQ, node);
                PCB_processes_count++;
            }
        }
    }
}

void handlerRRUSR1(int sigNum)
{
    int currentTime = getClk();
    int TA = currentTime - runningProcess->arrival_time;                              // turn around time
    float WTA = (float)TA / runningProcess->burst_time;                               // weighted turnaround time
    int WT = currentTime - runningProcess->arrival_time - runningProcess->burst_time; // waiting time
    accumulativeBT += runningProcess->burst_time;
    runningProcess->state = FINISHED;
    runningProcess->remaining_time = 0;
    runningProcess->waiting_time = WT;

    append_log_file(currentTime,
                    runningProcess->process_id,
                    runningProcess->state,
                    runningProcess->arrival_time,
                    runningProcess->burst_time,
                    runningProcess->remaining_time,
                    runningProcess->waiting_time,
                    TA,
                    WTA);

    accumulativeWTA += WTA;
    accumulativeWTAsquared += (WTA * WTA);
    accumulativeWT += WT;

    finishedProcesses++;
    runningProcess = NULL;

    //printf("recieved count %d, finished count %d , messagesEnded %d , QueueEmpty %d\n", PCB_processes_count, finishedProcesses, messagesEnded, (RRQ->head) ? 0 : 1);
    if (!RRQ->head && messagesEnded && (finishedProcesses == PCB_processes_count))
    {
        //printf("In the Condition\n");
        float cpu_utilization = 100 * ((float)accumulativeBT / (getClk()));
        float avg_WTA = (float)accumulativeWTA / PCB_processes_count;
        float avg_waiting = (float)accumulativeWT / PCB_processes_count;
        float std_WTA = sqrt((double)accumulativeWTAsquared / PCB_processes_count -
                             (double)(accumulativeWTA / PCB_processes_count) *
                                 (accumulativeWTA / PCB_processes_count));

        FILE *file = fopen(roundRobin_statistics_filename, "w");
        if (file == NULL)
            printf("Error opening %s", roundRobin_statistics_filename);
        fprintf(file, "CPU utilization = %.2f%% \nAvg WTA = %.2f \nAvg Waiting = %.2f \nStd WTA = %.2f ",
                cpu_utilization, avg_WTA, avg_waiting, std_WTA);
        fclose(file);

        destroyClk(false);
        exit(0);
    }
}

void handlerRRSigAlarm(int sigNum)
{
    signal(SIGALRM, handlerRRSigAlarm);
}

void append_log_file(int currentTime, int id, char *state, int arrival, int burst, int remaining, int waiting, int TA, float WTA)
{
    FILE *file = fopen(roundRobin_output_filename, "a");
    if (file == NULL)
        printf("Error opening %s", roundRobin_output_filename);

    if (state == "finished")
        fprintf(file, "#At time \t %d \t process \t %d \t %s \t arr \t %d \t total \t %d \t remain \t %d \t wait \t %d \t TA \t %d \t WTA \t %.2f \n",
                currentTime, id, state, arrival, burst, remaining, waiting, TA, WTA);
    else
        fprintf(file, "#At time \t %d \t process \t %d \t %s \t arr \t %d \t total \t %d \t remain \t %d \t wait \t %d \n",
                currentTime, id, state, arrival, burst, remaining, waiting);

    fclose(file);
}

//=========================================================HPF=====================================================================================
void handlerHPFUSR1(int sigNum)
{
    // As long as I am receiving processes this loop will continue looping
    //updated nowait
    //printf("entered handler1\n");
    while (msgrcv(ArrivalQueueID, &newProcess, sizeof(newProcess) - sizeof(newProcess.mtag), NEW_PROCESS_TAG, IPC_NOWAIT) != -1)
    {
        //printf("entered handler2\n");

        if (newProcess.process_id == -1)
        {
            // -1 means i reacived last process
            //I need to check if there are still remaining processes
            //case1: if there is a remaining process, then i will just break, and set a bool endofprocess
            //case2: if there is no remaining processes then i will end the SIMULATION.
            if (remaining_processes > 0 || remaining_processes == 0 && running)
            {
                endProcesses = true;
                break;
            }
            else
            {
                //THE FINAL PROCESS INDIDCATOR

                //CALCULATION OF AVG WTA, AVG WAITING AND STD WTA

                float AVG_TOTAL_WTA = WTA / count_HPF;
                float AVG_WAIT_VAL = Total_Waiting_Time / count_HPF;
                float number = (TOTAL_WTA_squared / count_HPF) - (TOTAL_WTA / count_HPF) * (TOTAL_WTA / count_HPF);

                //CALCULATE STD
                float temp = number / 2;
                float STD_TOTAL_WTA = (number / temp + temp) / 2;

                float utilization = 100 * ((float)TOTAL_BURST_HPF / (getClk() - 1));

                //UPDATE THE SCHEDULER.PERF FILE WITH FINAL STATS OF ALGORITHM
                fp = fopen("scheduler.perf", "w");
                if (fp == NULL)
                {
                    printf("Error in file creation\n");
                }
                else
                {
                    fprintf(fp, "CPU Utilization = %.2f%%\nAvg_WTA = %.2f\nAvg_Waiting : %.2f\nstd WTA : %.2f\n", utilization, AVG_TOTAL_WTA, AVG_WAIT_VAL, STD_TOTAL_WTA);
                }
                fclose(fp);

                //FREE OF RESOURCES
                destroyClk(true);
                exit(0);
            }
        }
        else
        {

            //Message received
            // Here we created a priority queue of all the messages received
            if (remaining_processes == 0)
            {
                //create a new node and a assign PCB Data to it.
                //This is the head of the message queue
                pq_HPF = newNode_HPF(newProcess.process_id, newProcess.priority, newProcess.arrival_time, newProcess.burst_time, newProcess.priority, newProcess.process_id);

                //we couldn't set the ID Here because we didnt fork yet and I can't use fork here
                //because I am not forking several processes at same time [according to the priority]

                //gives an number of processes
                count_HPF++;
                //THIS IS USED TO INDICATE I RECEIVED A PROCESS
                remaining_processes = 1;
            }
            else if (remaining_processes > 0)
            {
                //Incase a queue was already implemented so we only Enqueue into the queue
                Enqueue_HPF(&pq_HPF, newProcess.process_id, newProcess.priority, newProcess.arrival_time, newProcess.burst_time, newProcess.priority, newProcess.process_id);
                count_HPF++;
                //THIS IS USED TO INDICATE I RECEIVED A PROCESS
                remaining_processes++;
            }
        }
    }
    signal(SIGUSR1, handlerHPFUSR1);
}
