
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>

class NBCounter {
private:
	pthread_mutex_t mutex;
public:
	int x;

	NBCounter(int k = 0);
	~NBCounter();

	void increment();

	int get();

	void set(int k);

	void* x_address();
};


