#ifndef __THREADS_STATS_H__
#define __THREADS_STATS_H__


typedef struct Threads_stats *threads_stats;
void initStats(threads_stats stats, int id);
int getId(threads_stats stats);
int getTotalReq(threads_stats stats);
void incTotalReq(threads_stats stats);
int getStaticReq(threads_stats stats);
void incStaticReq(threads_stats stats);
int getDynamicReq(threads_stats stats);
void incDynamicReq(threads_stats stats);




#endif
