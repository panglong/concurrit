
#include "multiset.h"

void lock(element_t* elt) {
	pthread_mutex_lock(&elt->mutex);
}

void unlock(element_t* elt) {
	pthread_mutex_unlock(&elt->mutex);
}

void multiset_init(multiset_t* ms) {
	int status, i = 0;
	for(i = 0; i < MS_SIZE; ++i) {
		ms->elements[i].value = -1;
		ms->elements[i].valid = 0;
		status = pthread_mutex_init(&ms->elements[i].mutex, NULL);
		if (status != 0) exit(-1);
	}
}

int multiset_allocate(multiset_t* ms, int value) {
	int i = 0;
	for(i = 0; i < MS_SIZE; ++i) {
		lock(&ms->elements[i]);
		if(ms->elements[i].value == -1 && ms->elements[i].valid == 0) {
			ms->elements[i].value = value;
			unlock(&ms->elements[i]);
			return i;
		}
		unlock(&ms->elements[i]);
	}
	return -1;
}


int multiset_insert_pair(multiset_t* ms, int value1, int value2) {
	int index1 = -1, index2 = -1;

	index1 = multiset_allocate(ms, value1);
	if(index1 < 0) {
		return 0;
	}

	//----------------------------------------

	index2 = multiset_allocate(ms, value2);
	if(index2 < 0) {
		lock(&ms->elements[index1]);
			concurritAssert(ms->elements[index1].valid == 0);
			ms->elements[index1].value = -1;
		unlock(&ms->elements[index1]);
		return 0;
	}

	//----------------------------------------

	lock(&ms->elements[index1]);
	lock(&ms->elements[index2]);

		concurritAssert(ms->elements[index1].value != -1);
		concurritAssert(ms->elements[index1].valid == 0);
		ms->elements[index1].valid = 1;

		concurritAssert(ms->elements[index2].value != -1);
		concurritAssert(ms->elements[index2].valid == 0);
		ms->elements[index2].valid = 1;

	unlock(&ms->elements[index1]);
	unlock(&ms->elements[index2]);

	return 1;
}
