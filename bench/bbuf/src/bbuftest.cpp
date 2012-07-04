#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(BBScenario, "Bounded buffer scenario")

	TESTCASE() {
		CALL_TEST(SearchInFunc2);
	}

	//============================================================//

	TEST(SearchAll) {

		MAX_WAIT_TIME(3*USECSPERSEC);

		WHILE_STAR {
			TVAR(t);
			SELECT_THREAD_BACKTRACK(t, (), PTRUE, "Select");
			RUN_THREAD_UNTIL_NEXT(t, "Next");
			RUN_THREAD_UNTIL(t, READS() || WRITES() || CALLS() || HITS_PC() || ENDS(), "Run t");
		}
	}

	//============================================================//

	// GLOG_v=0 scripts/run_bench.sh bbuf -s -c
	// finfile: bounded_buf_get bounded_buf_put
	TEST(SearchInFunc1) {

		FVAR(f_get, bounded_buf_get);
		FVAR(f_put, bounded_buf_put);

		TVAR(t1);
		TVAR(t2);
		TVAR(t3);
		TVAR(t4);

		MAX_WAIT_TIME(0);

		WAIT_FOR_THREAD(t1, IN_FUNC(f_get), "Select t1");
		WAIT_FOR_DISTINCT_THREAD(t2, (t1), IN_FUNC(f_get), "Select t2");

		WAIT_FOR_DISTINCT_THREAD(t3, (t1, t2), IN_FUNC(f_put), "Select t3");
		WAIT_FOR_DISTINCT_THREAD(t4, (t1, t2, t3), IN_FUNC(f_put), "Select t4");

		MAX_WAIT_TIME(USECSPERSEC);

		WHILE(!HAVE_ENDED(t1, t2, t3, t4)) {

			TVAR(t);
			SELECT_THREAD_BACKTRACK(t, (t1, t2, t3, t4), PTRUE, "Select t");

			RUN_THREAD_THROUGH(t, READS() || WRITES() || CALLS() || HITS_PC() || ENDS(), "Run t until...");
		}
	}

	//============================================================//

	// GLOG_v=0 scripts/run_bench.sh bbuf -s -c
	// finfile: bounded_buf_get bounded_buf_put
	TEST(SearchInFunc2) {

		FVAR(f_get, bounded_buf_get);

		TVAR(t1);
		TVAR(t2);

		MAX_WAIT_TIME(0);

		WAIT_FOR_THREAD(t1, ENTERS(f_get), "Select t1");
		WAIT_FOR_DISTINCT_THREAD(t2, (t1), ENTERS(f_get), "Select t2");

		MAX_WAIT_TIME(USECSPERSEC);

		WHILE(!HAS_ENDED(t1) && !HAS_ENDED(t2)) {

			TVAR(t);
			SELECT_THREAD_BACKTRACK(t, (t1, t2), PTRUE, "Select t");

			RUN_THREAD_THROUGH(t, READS() || WRITES() || CALLS() || HITS_PC() || ENDS(), "Run t until...");
		}
	}

CONCURRIT_END_TEST(BBScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
