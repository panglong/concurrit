
#include "increment.h"
#include "dummy.h"

//int x = 0;

NBCounter::NBCounter(int k /*= 0*/) {
	x = k;
	int status = pthread_mutex_init(&mutex, NULL);
	if (status != 0) exit(-1);
}

NBCounter::~NBCounter() {
	pthread_mutex_destroy(&mutex);
}

void NBCounter::increment() {

	concurritStartInstrument();

	while(true) {
		long int t = x;
		long int k = t + 1;

		printf("---------------------- INCREMENT READS FROM X\n");

		lock();
		if(x == t) {
			x = k;
			unlock();
			printf("---------------------- INCREMENT SUCCEEDS\n");
			break;
		}
		unlock();
		printf("---------------------- INCREMENT RETRYING\n");

	}

	concurritEndInstrument();
}

int NBCounter::get() {
	return x;
}

void NBCounter::set(int k) {
	x = k;
}

void* NBCounter::x_address() {
	return (&x);
}

void NBCounter::lock() {
	pthread_mutex_lock(&mutex);
}

void NBCounter::unlock() {
	pthread_mutex_unlock(&mutex);
}

