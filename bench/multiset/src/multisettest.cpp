#include <stdio.h>

#include "multiset.h"

#define NUM_THREADS 3

#include "concurrit.h"


void* insertpair_routine1(void* arg)
{

  multiset_t * multiset = (multiset_t*) arg;

  multiset_insert_pair(multiset, 1, 2);

  return NULL;
}

void* insertpair_routine2(void* arg)
{

  multiset_t * multiset = (multiset_t*) arg;

  multiset_insert_pair(multiset, 3, 4);

  return NULL;
}

void* insertpair_routine3(void* arg)
{

  multiset_t * multiset = (multiset_t*) arg;

  multiset_insert_pair(multiset, 5, 6);

  return NULL;
}

void* lookup_routine1(void* arg)
{

  multiset_t * multiset = (multiset_t*) arg;

  int ret1 = multiset_lookup(multiset, 1);
  int ret2 = multiset_lookup(multiset, 2);

  concurritAssert(!ret1 || ret2);

  return NULL;
}

void* lookup_routine2(void* arg)
{

  multiset_t * multiset = (multiset_t*) arg;

  int ret1 = multiset_lookup(multiset, 3);
  int ret2 = multiset_lookup(multiset, 4);

  concurritAssert(!ret1 || ret2);

  return NULL;
}

void* lookup_routine3(void* arg)
{

  multiset_t * multiset = (multiset_t*) arg;

  int ret1 = multiset_lookup(multiset, 5);
  int ret2 = multiset_lookup(multiset, 6);

  concurritAssert(!ret1 || ret2);

  return NULL;
}



CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(INCScenario, "Multiset scenario")


	//---------------------------------------------
	multiset_t multiset;
	//---------------------------------------------

	SETUP() {
		srand(time(NULL));
		multiset_init(&multiset);
	}

	//---------------------------------------------

	TEARDOWN() {

	}

	TESTCASE() {

		ThreadVarPtr t_ip1 = CREATE_THREAD(insertpair_routine1, (void*)&multiset);
		ThreadVarPtr t_ip2 = CREATE_THREAD(insertpair_routine2, (void*)&multiset);
		ThreadVarPtr t_ip3 = CREATE_THREAD(insertpair_routine3, (void*)&multiset);

		ThreadVarPtr t_lu1 = CREATE_THREAD(lookup_routine1, (void*)&multiset);
//		ThreadVarPtr t_lu2 = CREATE_THREAD(lookup_routine2, (void*)&multiset);
//		ThreadVarPtr t_lu3 = CREATE_THREAD(lookup_routine3, (void*)&multiset);

		//-----------------------------------------

		MAX_WAIT_TIME(USECSPERSEC);

		FUNC(f_insertpair, multiset_insert_pair);
		FUNC(f_allocate, multiset_allocate);

		//-----------------------------------------

//		EXISTS(t1, PTRUE, "Select thread t1");
//		EXISTS(t2, NOT(t1), "Select thread t2");
//
//		WHILE_STAR
//		{
//			FORALL(t, (TID == t1 || TID == t2), "Select thread t");
//			RUN_UNTIL(BY(t), READS() || WRITES() || ENDS(), __, "Run until ends");
//		}

//		WHILE_STAR {
//			FORALL(t, PTRUE);
//			WHILE_STAR {
//				RUN_UNTIL(BY(t), READS() || WRITES() || ENDS(), __);
//			}
//		}


//		WHILE_STAR {
//			FORALL(t, PTRUE);
//			WHILE_STAR {
//				RUN_UNTIL(BY(t), IN_FUNC(f_insertpair) && (READS() || WRITES() || ENDS()), __);
//			}
//		}


		FORALL(t1, PTRUE);
		FORALL(t2, NOT(t1));

		WHILE_STAR {
			RUN_UNTIL(BY(t1), (AT_PC(1) || READS() || WRITES() || ENDS()), __);
		}

		WHILE_STAR {
			RUN_UNTIL(BY(t2), (AT_PC(1) || READS() || WRITES() || ENDS()), __);
		}

//		WAIT_FOR_ALL(0);
//		concurritAssert(multiset.size <= 2*NUM_THREADS);
//		concurritAssert(multiset.size%2 == 0);

	}

CONCURRIT_END_TEST(INCScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
