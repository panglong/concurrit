#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(BBScenario, "Bounded buffer scenario")

	TESTCASE() {
		CALL_TEST(Final);
	}

	TEST(Final) {

		FVAR(f_get, bounded_buf_get);
		FVAR(f_put, bounded_buf_put);

		MAX_WAIT_TIME(0);

		WAIT_FOR_THREAD(t1, IN_FUNC(f_get), "Select t1");
		WAIT_FOR_DISTINCT_THREAD(t2, IN_FUNC(f_get), "Select t2");

//		EXISTS(t1, IN_FUNC(f_get), "Select t1");
//		EXISTS(t2, IN_FUNC(f_get) && NOT(t1), "Select t2");

		MAX_WAIT_TIME(USECSPERSEC);

		WHILE_STAR {
			SELECT_THREAD_BACKTRACK(t, BY(t1) || BY(t2), "Select t");

			RUN_THREAD_UNTIL(BY(t), READS() || WRITES() || HITS_PC() || ENDS(), "Run t until...");
		}
	}

CONCURRIT_END_TEST(BBScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
