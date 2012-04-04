#include <stdio.h>

#include "increment.h"

#define NUM_THREADS 2

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

void* increment_routine(void* arg)
{

  NBCounter * counter = (NBCounter*) arg;

  safe_assert(counter != NULL);
  counter->increment();

  return NULL;
}

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(INCScenario, "NB-Increment scenario")

	SETUP() {
		counter = new NBCounter();
	}

	TEARDOWN() {
		if(counter != NULL)
			delete counter;
		counter = NULL;
	}

	NBCounter* counter;
	coroutine_t threads[NUM_THREADS];

	TESTCASE() {

		TEST_FORALL();

		for (int i = 0; i < NUM_THREADS; i++)
		{
			threads[i] = CREATE_THREAD(i, increment_routine, (void*)counter, true);
		}


		while(STAR)
		{
//				EXISTS(t);
//				DSLTransition(TransitionPredicate::True(), t);

//			DSLTransition(TransitionPredicate::True());

			EXISTS(t);
			DSLTransferUntil(t, ENDS());
		}
	}

CONCURRIT_END_TEST(INCScenario)
//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
