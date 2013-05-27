#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(BBScenario, "Bounded buffer scenario")

	TESTCASE() {
		MAX_WAIT_TIME(0);

		FUNCT(bounded_buf_get);
		FUNCT(bounded_buf_put);

		TVAR(P1);
		TVAR(C1);
		TVAR(C2);

		WAIT_FOR_THREAD(P1, ENTERS(bounded_buf_put), "Wait for a producer.");

		WAIT_FOR_DISTINCT_THREADS((C1, C2), ENTERS(bounded_buf_get), "Wait for 2 consumers.");

		RUN_THREAD_THROUGH(P1, RETURNS(bounded_buf_put), "Producer inserts an item.");

		RUN_THREAD_THROUGH(C1, HITS_PC(42), "First consumer runs first phase.");

		RUN_THREAD_THROUGH(C2, RETURNS(bounded_buf_get), "Second consumer removes the item.");

		// ERROR!
		RUN_THREAD_THROUGH(C1, ENDS(), "First consumer runs the second phase.");

	}

CONCURRIT_END_TEST(BBScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
