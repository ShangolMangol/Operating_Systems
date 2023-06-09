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

// HW3: Parse the new arguments too
void getargs(int *port, int* threadsNum, int* queueSize, int argc, char *argv[])
{
    if (argc < 2)
    {
	    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	    exit(1);
    }
    *port = atoi(argv[1]);
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
    pthread_mutex_init(&wa)
    pthread_t threadsPool = malloc(threadsNum * sizeof (pthread_t));
    for (int i = 0; i < threadsNum; ++i)
    {
        pthread_create(&(threadsPool[i]), NULL, parse_routine, NULL);
    }

    listenfd = Open_listenfd(port);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

    waiting_queue

    if(threadsNum <= running_queue->size)
    {
        pthread_cond_signal(&cond)
    }

	//
	// HW3: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	// 
	requestHandle(connfd);

//	Close(connfd);
    }

}


    


 
