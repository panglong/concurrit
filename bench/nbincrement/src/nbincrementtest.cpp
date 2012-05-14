#include <stdio.h>

#include "increment.h"

#define NUM_THREADS 3

#include "concurrit.h"


void* increment_routine(void* arg)
{

  NBCounter * counter = (NBCounter*) arg;

  safe_assert(counter != NULL);

  counter->increment();

  return NULL;
}


CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(INCScenario, "NB-Increment scenario")

	SETUP() {
		safe_assert(counter == NULL);
		counter = new NBCounter();
	}

	//---------------------------------------------

	TEARDOWN() {
		if(counter != NULL)
			delete counter;
		counter = NULL;
	}

	//---------------------------------------------
	NBCounter* counter;
	//---------------------------------------------

	TESTCASE() {

		MAX_WAIT_TIME(3*USECSPERSEC);

		for (int i = 0; i < NUM_THREADS; i++)
		{
			CREATE_THREAD(i+1, increment_routine, (void*)counter);
		}

		//-----------------------------------------

		printf("counter->x: %lx\n", &counter->x);
		printf("counter->lock: %lx\n", &counter->mutex);

		EXISTS(t1, PTRUE, "Select thread t1");
		EXISTS(t2, NOT(t1), "Select thread t2");
//
		RUN_UNTIL(BY(t1), ENDS(), __, "Reads from x");
//
//		RUN_UNTIL(NOT_BY(t1), WRITES(&counter->x, t2), __, "Writes to x");
//
//		RUN_UNTIL(PTRUE, ENDS(t1), __, "Ends...");


//		WHILE_STAR
//		{
//			FORALL(t, (TID == t1 || TID == t2), "Select thread t");
//			RUN_UNTIL(BY(t), ENDS(), __, "Run until ends");
//		}



//			FORALL(t, PTRUE, "Select thread t");
//			RUN_UNTIL(BY(t), ENDS(), __, "Run until ends");



//			FORALL(t, PTRUE, "Select thread t");
//			RUN_UNTIL(BY(t), PTRUE, __, "Run until ends");
//		}

	}

CONCURRIT_END_TEST(INCScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
