
#include "increment.h"

NBCounter::NBCounter(int k /*= 0*/) {
	x = k;
	int status = pthread_mutex_init(&mutex, NULL);
	if (status != 0) exit(-1);
}

NBCounter::~NBCounter() {
	pthread_mutex_destroy(&mutex);
}

void NBCounter::increment() {

	while(true) {
		int t = x;
		int k = t + 1;

		pthread_mutex_lock(&mutex);
		if(x == t) {
			x = k;
			pthread_mutex_unlock(&mutex);
			break;
		}
		pthread_mutex_unlock(&mutex);
		printf("Looping...\n");
	}

}

int NBCounter::get() {
	return x;
}

void NBCounter::set(int k) {
	x = k;
}

