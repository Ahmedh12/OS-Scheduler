#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300

//=================================================Ahmed Hussien=====================================================================
#define ARRIVAL_QUEUE_ID 12345    //ID used to create the queue for communication between the processes generator and the scheduler
struct generator_shedular_msg     //message structure to send a process from the generator to the scheduler
{
    long int mtag;
    int process_id;
    int arrival_time;
    int burst_time;
    int priority;
};
struct process_msg //message structure to send a process from the generator to the scheduler
{
    long process_id;
    int time; //burst time from scheduler to process, remaining time from process to scheduler
};

#define NEW_PROCESS_TAG 121

// ======================================= Millania Sameh =======================================

// States
#define RUNNING "started"
#define WAITING "stopped"
#define FINISHED "finished"

// Queue data structure
// Node
struct Node
{
    struct Node *next;
    int process_id; //simulation process id
    int arrival_time;
    int burst_time; //  = running time = execution time
    int priority;
    char *state;
    int waiting_time;
    int remaining_time;
    int pid; //PID due to forking (7etta 5alooda) system process id
    bool started; //7etta 5aloodalooda
};

struct Node *construct_node(int id, int arrival, int burst, int priority, int pid)
{
    struct Node *node = (struct Node *)malloc(sizeof(struct Node));
    node->process_id = id;
    node->arrival_time = arrival;
    node->burst_time = burst;
    node->priority = priority;
    node->next = NULL;
    node->state = WAITING;
    node->waiting_time = 0;
    node->remaining_time = burst;
    node->pid = pid;
    bool started = false;
    return node;
}

// Queue
struct Queue
{
    struct Node *head;
    struct Node *tail;
};

struct Queue *construct_queue()
{
    struct Queue *queue = (struct Queue *)malloc(sizeof(struct Queue));
    queue->head = NULL;
    queue->tail = NULL;
    return queue;
}

bool is_queue_empty(struct Queue *queue)
{
    return (queue->head == NULL);
}

void enqueue(struct Queue *queue, struct Node *node)
{
    // enqueue at the end of the queue

    if (is_queue_empty(queue))
        queue->head = node;
    else
        queue->tail->next = node;
    queue->tail = node;
}

//7etta s7s
struct Node* DeepCopy(struct Node* src) 
{
    struct Node* dest = (struct Node*) malloc(sizeof(struct Node));
    dest->arrival_time = src->arrival_time;
    dest->burst_time = src->burst_time;
    dest->next = src->next;
    dest->pid = src->pid;
    dest->priority = src->priority;
    dest->process_id = src->process_id;
    dest->remaining_time = src->remaining_time;
    dest->started = src->started;
    dest->state = src->state;
    dest->waiting_time = src->waiting_time;
    return dest;
}

//7etta 5alooda bardo
bool EnqueuePriority(struct Queue *q, int id, int arrival, int burst, int priority, int pid)
{
    struct Node *node = (struct Node *)malloc(sizeof(struct Node));
    struct Node *tail = q->head;

    node->process_id = id;
    node->arrival_time = arrival;
    node->burst_time = burst;
    node->priority = priority;
    node->next = NULL;
    node->state = WAITING;
    node->waiting_time = 0;
    node->remaining_time = burst;
    node->pid = pid;
    node->started = false;

    if (tail)
    {
        if (node->burst_time < tail->remaining_time)
        {
            q->head = node;
            node->next = tail;
            return true;
        }
        while (tail->next && (node->burst_time >= tail->next->remaining_time))
        {
            tail = tail->next;
        }
        struct Node *temp = tail->next;
        tail->next = node;
        node->next = temp;
    }
    else
    {
        q->head = node;
    }
    return false;
}

void dequeue(struct Queue *queue)
{
    struct Node *node_to_be_deleted = queue->head;
    queue->head = queue->head->next;
    if (node_to_be_deleted == queue->tail)
        queue->tail = NULL;
    free(node_to_be_deleted);
}

void remove_node(struct Queue *queue, struct Node *prev_node, struct Node *curr_node) // remove this
{
    // =============================> TODO: check special case only one node

    if (!is_queue_empty(queue))
    {
        prev_node->next = curr_node->next;
        free(curr_node);
    }
}

//================================================ Ahmed Tawheed ==================================================// 

// priority Node_HPF
typedef struct node {
   int data;
   struct node* next;
    // status:
    // 0:running, 1:waiting/blocked, 2:Finished
    int status;
    //time entered the queue
    int Arrival_time;
    //running time of the queue
    int Burst_time;
    //priority
    int priority;
    //Process_id given to child in forking
    int pid;

    //Process_id_read from the file
    int Process_id_read;

    //Time since process/child entered the priority Queue
    //start running
    int start_time;

    //Time process/child waiting  in the priority Queue
    int waiting_time;

    //Used for calculation purposes in calculating time durations:
    int end_time;
} Node_HPF;


typedef struct PCB_Current
{
   // status:
    // 0:running, 1:waiting/blocked, 2:Finished
    int status;
    //time entered the queue
    int Arrival_time;
    //running time of the queue
    int Burst_time;
    //priority
    int priority;
    //Process_id given to child in forking
    int pid;
    //Process_id_read from the file
    int Process_id_read;
    //Time since process/child entered the priority Queue
    //start running
    int start_time;
    //Time process/child waiting  in the priority Queue
    int waiting_time;
    //Used for calculation purposes in calculating time durations:
    int end_time;
} PCB_Current;



struct msgbuff
{
    long mtype;
    char mtext[7];
};


Node_HPF* newNode_HPF(int d, int p,int arrival_time,int burst_time,int priority,int Process_id_read) {
   Node_HPF* temp = (Node_HPF*)malloc(sizeof(Node_HPF));
   temp->data = d;
   temp->priority = p;
   temp->next = NULL;
   //added
    temp->status = 1; //Waiting
    temp->Arrival_time = arrival_time;
    temp->Burst_time = burst_time;
    temp->Process_id_read=Process_id_read;
    temp->priority = priority;
    temp->end_time = 0;
    temp->pid = 0;
    temp->start_time = 0;
    temp->waiting_time = 0;
   return temp;
}

int peek(Node_HPF** head) {
   return (*head)->Process_id_read;
}

void Dequeue_HPF(Node_HPF** head) {
   Node_HPF* temp = *head;
   (*head) = (*head)->next;
   free(temp);
}
void Enqueue_HPF(Node_HPF** head,int d, int p,int arrival_time,int burst_time,int priority,int Process_id_read) {
   Node_HPF* start = (*head);
   Node_HPF* temp = newNode_HPF(d, p, arrival_time,burst_time,priority,Process_id_read);
   //lower value has highest priority
   if ((*head)->priority > p) {
      temp->next = *head;
      (*head) = temp;
   } else {
      while (start->next != NULL &&
      start->next->priority < p) {
         start = start->next;
      }
      // Either at the ends of the list
      // or at required position
      temp->next = start->next;
      start->next = temp;
   }
}
// Function to check the queue is empty
int isEmpty(Node_HPF** head) {
   return (*head) == NULL;
}

//======================================================================================================================//



///==============================
//don't mess with this variable//
int *shmaddr; //
//===============================

int getClk()
{
    return *shmaddr;
}

/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
}

/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 * It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}


char* convertIntegerToChar(int N)
{
 
    // Count digits in number N
    int m = N;
    int digit = 0;
    while (m) {
 
        // Increment number of digits
        digit++;
 
        // Truncate the last
        // digit from the number
        m /= 10;
    }
 
    // Declare char array for result
    char* arr;
 
    // Declare duplicate char array
    char arr1[digit];
 
    // Memory allocation of array
    arr = (char*)malloc(digit);
 
    // Separating integer into digits and
    // accommodate it to character array
    int index = 0;
    while (N) {
 
        // Separate last digit from
        // the number and add ASCII
        // value of character '0' is 48
        arr1[++index] = N % 10 + '0';
 
        // Truncate the last
        // digit from the number
        N /= 10;
    }
 
    // Reverse the array for result
    int i;
    for (i = 0; i < index; i++) {
        arr[i] = arr1[index - i];
    }
 
    // Char array truncate by null
    arr[i] = '\0';
 
    // Return char array
    return (char*)arr;
}
