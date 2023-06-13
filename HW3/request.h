#ifndef __REQUEST_H__

void requestHandle(int fd, struct timeval arrivalTime, struct timeval dispatchTime, int threadId);

#endif
