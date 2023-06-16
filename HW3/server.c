#include "segel.h"
#include "request.h"


#define BLOCK 1
#define DROP_TAIL 2
#define DROP_HEAD 3
#define BLOCK_FLUSH 4
#define DYNAMIC 5
#define DROP_RANDOM 6


//
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

//
//pthread_mutex_t waiting_mutex;
//pthread_mutex_t running_mutex;
pthread_mutex_t queue_mutex;
pthread_cond_t thread_cond;
pthread_cond_t wait_master_cond;
pthread_cond_t empty_queue_cond;
int schedAlg;


//** queue functions **/
typedef struct Node{
    int fd;
    struct timeval arrival_time;
    struct Node* next;
} Node;

typedef struct Queue {
    Node* front;
    Node* rear;
    int size;
} Queue;

void setup_Node(Node* node, int fd1)
{
    node->fd = fd1;

    node->next = NULL;
}

void set_up_stat(stat_data* thread_stat, int thread_id){
    thread_stat->threadId = thread_id;
    thread_stat->jobCount = 0;
    thread_stat->staticJobCount = 0;
    thread_stat->dynamicJobCount = 0;
}
struct Queue waiting_queue;
struct Queue running_queue;
void remove_by_index(struct Queue* queue, int index);


void setup_queue(Queue* queue) {
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
}

int get_size(Queue* queue){
    return queue->size;
}

void enqueue_rear(Queue* queue, int fd){
    Node* node = (Node*)malloc (sizeof(Node));
    setup_Node(node, fd);
    if(queue->size == 0)
    {
        queue->front = node;
        queue->rear = node;
    }
    else
    {
        (queue->rear)->next = node;
        queue->rear = node;
    }
    (queue->size)++;
}


void enqueue_rear_start(Queue* queue, int fd, struct timeval time){
    Node* node = (Node*)malloc (sizeof(Node));
    setup_Node(node, fd);
    if(queue != &running_queue)
    {
        node->arrival_time = time;
    }
    if(queue->size == 0)
    {
        queue->front = node;
        queue->rear = node;
    }
    else
    {
        (queue->rear)->next = node;
        queue->rear = node;
    }
    (queue->size)++;
}


void dequeue_front(Queue* queue, int* result, struct timeval* time){
    Node* frontNode = NULL;
    *result = -1;
    if(queue->size > 0)
    {
        frontNode = queue->front;
        if(time != NULL) {
            *time = frontNode->arrival_time;
        }
        queue->front = (queue->front)->next;
	if(queue->front == NULL){
		queue->rear = NULL;
	}
        (queue->size)--;
        *result = frontNode->fd;
    }
    if(frontNode != NULL)
        free(frontNode);
}

void remove_by_fd(struct Queue* queue, int fd)
{
    Node* temp_node = queue->front;
    Node* prev_node = NULL;

    while(temp_node!=NULL)
    {
        if(temp_node->fd == fd)
        {
            if(prev_node == NULL)
            {
                queue->front = temp_node->next;
                if(queue->front == NULL)
                {
                    queue->rear = NULL;
                }
            }
            else
            {
                prev_node->next = temp_node->next;
                if (temp_node == queue->rear) {
                    queue->rear = prev_node;
                }

            }
            free(temp_node);
            (queue->size)--;
            break;
        }
        else
        {
            prev_node = temp_node;
            temp_node = temp_node->next;
        }
    }
}


void remove_by_index(Queue* queue, int index)
{
    if(index<0 || index >= queue->size){
        return;
    }
    Node* toDelete = queue->front;
    for(int i=0; i<index; i++)
    {
        toDelete = toDelete->next;
    }
    int closeId = toDelete->fd;
    remove_by_fd(queue, closeId);
    Close(closeId);
}

void* parse_routine(void* arg)
{
    struct stat_data* threadData = (stat_data*) arg;
    while(1)
    {
        pthread_mutex_lock(&queue_mutex);
        while(waiting_queue.size == 0){
            pthread_cond_wait(&thread_cond, &queue_mutex);
        }

        int connectionFd;

        struct timeval arrivalTime;
        dequeue_front(&waiting_queue, &connectionFd, &arrivalTime);

        enqueue_rear(&running_queue, connectionFd);

        pthread_mutex_unlock(&queue_mutex);

        struct timeval pickupTime;
        struct timeval dispatchTime;
        gettimeofday(&pickupTime, NULL);

        timersub(&pickupTime, &arrivalTime, &dispatchTime);
        requestHandle(connectionFd, arrivalTime, dispatchTime, threadData);
        Close(connectionFd);
        pthread_mutex_lock(&queue_mutex);
        remove_by_fd(&running_queue, connectionFd);
        pthread_mutex_unlock(&queue_mutex);


        if(schedAlg == BLOCK)
        {
            pthread_cond_signal(&wait_master_cond);

        } else if(schedAlg == BLOCK_FLUSH)
        {
            pthread_mutex_lock(&queue_mutex);
            if(waiting_queue.size + running_queue.size == 0)
            {
                pthread_cond_signal(&wait_master_cond);
            }
            pthread_mutex_unlock(&queue_mutex);

        }

    }
}


// HW3: Parse the new arguments too
void getargs(int *port, int* threadsNum, int* queueSize, int argc, char *argv[], int* maxSize)
{
    if (argc < 2)
    {
	    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	    exit(1);
    }
    *port = atoi(argv[1]);
    *threadsNum = atoi(argv[2]);
    *queueSize = atoi(argv[3]);
    char *schedAlgStr = argv[4];
    if(argc>5){
        *maxSize = atoi(argv[5]);
    }
    else{
        *maxSize = -1;
    }
    if(strcmp(schedAlgStr, "block")==0){
        schedAlg = BLOCK;
    }
    else if(strcmp(schedAlgStr, "dt") == 0){
        schedAlg = DROP_TAIL;
    }
    else if(strcmp(schedAlgStr, "dh") == 0){
        schedAlg = DROP_HEAD;
    }
    else if(strcmp(schedAlgStr, "bf") == 0){
        schedAlg = BLOCK_FLUSH;
    }
    else if(strcmp(schedAlgStr, "dynamic") == 0){
        schedAlg = DYNAMIC;
    }
    else if(strcmp(schedAlgStr, "random") == 0){
        schedAlg = DROP_RANDOM;
    }
}



int main(int argc, char *argv[]) {
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    int threadsNum, queueSize, maxSize;
    getargs(&port, &threadsNum, &queueSize, argc, argv, &maxSize);

    //
    // HW3: Create some threads...
    //

    pthread_cond_init(&thread_cond, NULL);
    pthread_cond_init(&wait_master_cond, NULL);
    setup_queue(&running_queue);
    setup_queue(&waiting_queue);

    pthread_mutex_init(&queue_mutex, NULL);
//    pthread_mutex_init(&waiting_mutex, NULL);
//    pthread_mutex_init(&running_mutex, NULL);
  //  pthread_mutex_init(&master_mutex, NULL);

    stat_data *threadStats = (stat_data*) malloc(threadsNum * sizeof(stat_data));
//    int *indexArr = (int *) malloc(threadsNum* sizeof(int));
    pthread_t *threadsPool = (pthread_t *) malloc(threadsNum * sizeof(pthread_t));
    for (int i = 0; i < threadsNum; ++i) {
//        indexArr[i] = i;
        set_up_stat(&(threadStats[i]), i);
        pthread_create(&(threadsPool[i]), NULL, parse_routine, (void*)(&(threadStats[i])) );
    }

    listenfd = Open_listenfd(port);
    struct timeval currTime;

    while (1) {
        clientlen = sizeof(clientaddr);

        connfd = Accept(listenfd, (SA *) &clientaddr, (socklen_t * ) & clientlen);
        gettimeofday(&currTime, NULL);

        pthread_mutex_lock(&queue_mutex);
        if (running_queue.size + waiting_queue.size < queueSize) {
//            pthread_mutex_unlock(&queue_mutex);

//            pthread_mutex_lock(&queue_mutex);

            enqueue_rear_start(&waiting_queue, connfd, currTime);

            if (running_queue.size < threadsNum) {
                pthread_cond_signal(&thread_cond);
            }

//            printf("here waiting\n");
            pthread_mutex_unlock(&queue_mutex);

        } else {
            if (schedAlg == BLOCK) {
//                printf("in block\n");

                pthread_cond_wait(&wait_master_cond, &queue_mutex);


                enqueue_rear_start(&waiting_queue, connfd, currTime);

                if (running_queue.size < threadsNum) {
                    pthread_cond_signal(&thread_cond);
                }

                pthread_mutex_unlock(&queue_mutex);


            } else if (schedAlg == DROP_TAIL) {
                Close(connfd);
                pthread_mutex_unlock(&queue_mutex);

            } else if (schedAlg == DROP_HEAD) {
                int tempRes;
                dequeue_front(&waiting_queue, &tempRes, NULL);
                if(tempRes != -1)
                    Close(tempRes);

                enqueue_rear_start(&waiting_queue, connfd, currTime);

                if (running_queue.size < threadsNum) {
                    pthread_cond_signal(&thread_cond);
                }

                pthread_mutex_unlock(&queue_mutex);

            } else if (schedAlg == BLOCK_FLUSH) {

                pthread_cond_wait(&wait_master_cond, &queue_mutex);

//                enqueue_rear_start(&waiting_queue, connfd, currTime);
                Close(connfd);
//                if (running_queue.size < threadsNum) {
//                    pthread_cond_signal(&thread_cond);
//                }

                pthread_mutex_unlock(&queue_mutex);


            } else if (schedAlg == DYNAMIC) {
                if (queueSize + 1 <= maxSize) {
                    queueSize++;
                }
                Close(connfd);
                pthread_mutex_unlock(&queue_mutex);


            } else if (schedAlg == DROP_RANDOM) {
                int taskToDrop;
                int dropSize = (waiting_queue.size + 1) / 2;
                for (int i = 0; i < dropSize; i++) {
                    taskToDrop = rand() % waiting_queue.size;
                    remove_by_index(&waiting_queue, taskToDrop);
                }
                enqueue_rear_start(&waiting_queue, connfd, currTime);
                pthread_mutex_unlock(&queue_mutex);
            }
        }

        //
        // HW3: In general, don't handle the request in the main thread.
        // Save the relevant info in a buffer and have one of the worker threads
        // do the work.
        //


    }

}






