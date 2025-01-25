#ifndef __REQUEST_H__
#define __REQUEST_H__

#include "segel.h"
#include "threads_stats.h"

void requestHandle(int fd, struct timeval arrival_time, struct timeval dispatch_time, threads_stats stats);

#endif
