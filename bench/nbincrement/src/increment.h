
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>

//extern int x;
class NBCounter {
public:
	pthread_mutex_t mutex;
	long int x;

	NBCounter(int k = 0);
	~NBCounter();

	void increment();

	int get();

	void lock();
	void unlock();

	void set(int k);

	void* x_address();
};


