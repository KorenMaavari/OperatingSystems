typedef struct Threads_stats{
	int id;
	int stat_req;
	int dynm_req;
	int total_req;
} *threads_stats;

void initStats(threads_stats stats, int id){
	stats->id = id;
	stats->stat_req = 0;
	stats->dynm_req = 0;
	stats->total_req = 0;
}

int getId(threads_stats stats){
   return stats->id;
}


int getTotalReq(threads_stats stats){
	return stats->total_req;
}

void incTotalReq(threads_stats stats){
	stats->total_req++;
}

int getStaticReq(threads_stats stats){
	return stats->stat_req;
}

void incStaticReq(threads_stats stats){
	stats->stat_req++;
}

int getDynamicReq(threads_stats stats){
	return stats->dynm_req;
}

void incDynamicReq(threads_stats stats){
	stats->dynm_req++;
}

