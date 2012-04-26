#include <stdio.h>

#include "multiset.h"

#define NUM_THREADS 3

#include "concurrit.h"


void* insertpair_routine(void* arg)
{

  multiset_t * multiset = (multiset_t*) arg;

  safe_assert(multiset != NULL);

  multiset_insert_pair(multiset, rand(), rand());

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

		MAX_WAIT_TIME(3*USECSPERSEC);

		FUNC(f_insertpair, multiset_insert_pair);
		FUNC(f_allocate, multiset_allocate);

		//-----------------------------------------

		for (int i = 0; i < NUM_THREADS; i++)
		{
			CREATE_THREAD(i+1, insertpair_routine, (void*)&multiset);
		}

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
		WHILE_STAR {
			RUN_UNTIL(BY(t1), IN_FUNC(f_insertpair) && (READS() || WRITES() || ENDS()), __);
		}

		FORALL(t2, NOT(t1));
		WHILE_STAR {
			RUN_UNTIL(BY(t2), IN_FUNC(f_insertpair) && (READS() || WRITES() || ENDS()), __);
		}

	}

CONCURRIT_END_TEST(INCScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
