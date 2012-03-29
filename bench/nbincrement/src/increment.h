
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>

class NBCounter {
private:
	volatile int x;
	pthread_mutex_t mutex;
public:
	NBCounter(int k = 0);
	~NBCounter();

	void increment();

	int get();

	void set(int k);
};


