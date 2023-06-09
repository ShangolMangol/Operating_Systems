#include "segel.h"
#include "request.h"

//
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//


pthread_mutex_t waiting_mutex;
pthread_mutex_t running_mutex;
pthread_cond_t cond;


//** queue functions **/
typedef struct Node{
    int fd;
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



struct Queue waiting_queue;
struct Queue running_queue;

void setup_queue(Queue* queue) {
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
}

int get_size(Queue* queue){
    return queue->size;
}

void enqueue_rear(Queue* queue, int fd, pthread_mutex_t* mutex){
    pthread_mutex_lock(mutex);
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
    pthread_mutex_unlock(mutex);
}


void dequeue_front_no_block(Queue* queue, int* result, pthread_mutex_t* mutex){
    Node* frontNode = NULL;
    *result = -1;
    if(queue->size > 0)
    {
        frontNode = queue->front;
        queue->front = (queue->front)->next;
        (queue->size)--;
        *result = frontNode->fd;
    }
    if(frontNode != NULL)
        free(frontNode);
    pthread_mutex_unlock(mutex);
}

void dequeue_front(Queue* queue, int* result, pthread_mutex_t* mutex){
    pthread_mutex_lock(mutex);
    dequeue_front_no_block(queue, result, mutex);
}

void remove_by_fd(Queue* queue, int fd, pthread_mutex_t *mutex){
    pthread_mutex_lock(mutex);
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

    pthread_mutex_unlock(mutex);
}

void* parse_routine(void* arg)
{
    while(1)
    {

        while(waiting_queue.size == 0){
            pthread_cond_wait(&cond, &waiting_mutex);
        }

        int connectionFd;
//        printf("starting in a new thread\n");
        dequeue_front_no_block(&waiting_queue, &connectionFd, &waiting_mutex);
   //     printf("passed dequeue_front_no_block\n");
        enqueue_rear(&running_queue, connectionFd, &running_mutex);
   //     printf("passed: enqueue_rear\n");
        requestHandle(connectionFd);
//        printf("passed requestHandle\n");
        remove_by_fd(&running_queue, connectionFd, &running_mutex);
//        printf("passed: remove_by_id\n");
        close(connectionFd);
//        printf("passed: close\n");


//        currentRequest = pop_from_queue(&waiting);
//        add_to_queue(&running);
//        requestHandle(currentRequest);
//        remove_by_fd(&running, currentRequest);
    }
}


// HW3: Parse the new arguments too
void getargs(int *port, int* threadsNum, int* queueSize, int argc, char *argv[])
{
    if (argc < 2)
    {
	    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	    exit(1);
    }
    *port = atoi(argv[1]);
    *threadsNum = atoi(argv[2]);
    *queueSize = atoi(argv[3]);
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;

    int threadsNum, queueSize;
    getargs(&port, &threadsNum, &queueSize, argc, argv);

    //
    // HW3: Create some threads...
    //

    pthread_cond_init(&cond, NULL);
    setup_queue(&running_queue);
    setup_queue(&waiting_queue);

    pthread_mutex_init(&waiting_mutex, NULL);
    pthread_mutex_init(&running_mutex, NULL);
    pthread_t* threadsPool = (pthread_t *) malloc(threadsNum * sizeof(pthread_t));
    for (int i = 0; i < threadsNum; ++i)
    {
        pthread_create(&(threadsPool[i]), NULL, parse_routine, NULL);
    }

    listenfd = Open_listenfd(port);
    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

        enqueue_rear(&waiting_queue, connfd ,&waiting_mutex);

        if(running_queue.size <= threadsNum)
        {
            pthread_cond_signal(&cond);
        }

        //
        // HW3: In general, don't handle the request in the main thread.
        // Save the relevant info in a buffer and have one of the worker threads
        // do the work.
        //

    //	requestHandle(connfd);

    //	Close(connfd);
    }

}






