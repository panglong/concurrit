
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
	ms->size = 0;
}

int multiset_lookup(multiset_t* ms, int value) {
	int i = 0;
	concurritFuncEnter(multiset_lookup);

	for(i = 0; i < MS_SIZE; ++i) {
		lock(&ms->elements[i]);
		if(ms->elements[i].value == value && ms->elements[i].valid == 1) {
			unlock(&ms->elements[i]);
			concurritFuncReturn(multiset_lookup);
			return 1;
		}
		unlock(&ms->elements[i]);
	}

	concurritFuncReturn(multiset_lookup);
	return 0;
}

int multiset_allocate(multiset_t* ms, int value) {
	int i = 0;
	concurritFuncEnter(multiset_allocate);

	for(i = 0; i < MS_SIZE; ++i) {
		lock(&ms->elements[i]);
		if(ms->elements[i].value == -1 && ms->elements[i].valid == 0) {
			ms->elements[i].value = value;
			unlock(&ms->elements[i]);
			concurritAtPc(1);
			concurritFuncReturn(multiset_allocate);
			return i;
		}
		unlock(&ms->elements[i]);
		concurritAtPc(1);
	}

	concurritFuncReturn(multiset_allocate);
	return -1;
}


int multiset_insert_pair(multiset_t* ms, int value1, int value2) {
	int index1 = -1, index2 = -1;

	concurritFuncEnter(multiset_insert_pair);

	index1 = multiset_allocate(ms, value1);
	if(index1 < 0) {

		concurritFuncReturn(multiset_insert_pair);
		return 0;
	}

	concurritAtPc(1);

	//----------------------------------------

	index2 = multiset_allocate(ms, value2);
	if(index2 < 0) {
		lock(&ms->elements[index1]);
			concurritAssert(ms->elements[index1].valid == 0);
			ms->elements[index1].value = -1;
		unlock(&ms->elements[index1]);

		concurritFuncReturn(multiset_insert_pair);
		return 0;
	}

	concurritAtPc(1);

	concurritAssert(index1 != index2);

	// ensure index1< index2 to avoid deadlock
	if(index2 < index1) {
		int tmp = index2;
		index2 = index1;
		index1 = tmp;
	}

	//----------------------------------------

//	lock(&ms->elements[index1]);
//	lock(&ms->elements[index2]);
//
//		concurritAssert(ms->elements[index1].value != -1);
//		concurritAssert(ms->elements[index1].valid == 0);
//		ms->elements[index1].valid = 1;
//		ms->size++;
//
//		concurritAssert(ms->elements[index2].value != -1);
//		concurritAssert(ms->elements[index2].valid == 0);
//		ms->elements[index2].valid = 1;
//		ms->size++;
//
//	unlock(&ms->elements[index1]);
//	unlock(&ms->elements[index2]);

	//----------------------------------------

	lock(&ms->elements[index1]);
		concurritAssert(ms->elements[index1].value != -1);
		concurritAssert(ms->elements[index1].valid == 0);
		ms->elements[index1].valid = 1;
		ms->size++;
	unlock(&ms->elements[index1]);

	concurritAtPc(1);

	lock(&ms->elements[index2]);
		concurritAssert(ms->elements[index2].value != -1);
		concurritAssert(ms->elements[index2].valid == 0);
		ms->elements[index2].valid = 1;
		ms->size++;
	unlock(&ms->elements[index2]);

	concurritFuncReturn(multiset_insert_pair);
	return 1;
}
