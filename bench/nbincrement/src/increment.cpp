
#include "increment.h"
#include "dummy.h"

NBCounter::NBCounter(int k /*= 0*/) {
	x = k;
	int status = pthread_mutex_init(&mutex, NULL);
	if (status != 0) exit(-1);
}

NBCounter::~NBCounter() {
	pthread_mutex_destroy(&mutex);
}

void NBCounter::increment() {

	StartInstrument();

	while(true) {
		int t = x;
		int k = t + 1;

		AtPc(42);

		printf("---------------------- INCREMENT READS FROM X\n");

		pthread_mutex_lock(&mutex);
		if(x == t) {
			x = k;
			pthread_mutex_unlock(&mutex);
			printf("---------------------- INCREMENT SUCCEEDS\n");
			break;
		}
		pthread_mutex_unlock(&mutex);
		printf("---------------------- INCREMENT RETRYING\n");
	}

	EndInstrument();
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

