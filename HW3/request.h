#ifndef __REQUEST_H__


typedef struct stat_data {
    int threadId;
    int jobCount;
    int staticJobCount;
    int dynamicJobCount;
} stat_data;

typedef struct complete_data {
    struct timeval arrival;
    struct timeval dispatch;
    struct stat_data* statData;
} complete_data;

void requestHandle(int fd, struct timeval arrivalTime, struct timeval dispatchTime, struct stat_data* thread_data);


#endif
