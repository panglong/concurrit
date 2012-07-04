#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {
		CALL_TEST(SearchLargeStep);
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

	// GLOG_v=1 scripts/run_bench.sh bzip2 -s -c -r
	TEST(SearchInFunc) {

		MAX_WAIT_TIME(3*USECSPERSEC);

		FUNC(f, threadFunction);
		FUNC(g, writerThread);

		TVAR(t1);
		TVAR(t2);
		TVAR(t3);

		WAIT_FOR_THREAD(t1, IN_FUNC(f), "t1");
		WAIT_FOR_DISTINCT_THREAD(t2, (t1), IN_FUNC(f), "t2");
		WAIT_FOR_DISTINCT_THREAD(t3, (t1, t2), IN_FUNC(g), "t3");

		WHILE(!HAVE_ENDED(t1, t2, t3)) {
			TVAR(t);
			SELECT_THREAD_BACKTRACK(t, (t1, t2, t3), PTRUE, "Select");
			RUN_THREAD_UNTIL_NEXT(t, "Next");
			RUN_THREAD_UNTIL(t, READS() || WRITES() || CALLS() || ENDS(), "Run t");
		}

	}

	//============================================================//

	// GLOG_v=1 scripts/run_bench.sh bzip2 -s -c -r -m1 -p0
	TEST(SearchLargeStep) {

		MAX_WAIT_TIME(3*USECSPERSEC);

		FUNC(f, threadFunction);
		FUNC(g, writerThread);

		TVAR(t1);
		TVAR(t2);
		TVAR(t3);

		WAIT_FOR_THREAD(t1, IN_FUNC(f), "t1");
		WAIT_FOR_DISTINCT_THREAD(t2, (t1), IN_FUNC(f), "t2");
		WAIT_FOR_DISTINCT_THREAD(t3, (t1, t2), IN_FUNC(g), "t3");

		WHILE(!HAVE_ENDED(t1, t2, t3)) {
			TVAR(t);
			SELECT_THREAD_BACKTRACK(t, (t1, t2, t3), PTRUE, "Select");

			RUN_THREAD_UNTIL_NEXT(t, "Next");
			RUN_THREAD_UNTIL(t, HITS_PC() || ENDS(), "Run t");
		}
	}

	//============================================================//

	// GLOG_v=1 scripts/run_bench.sh bzip2 -s -c -r -m1 -p0
	TEST(ExactScheduleTest) {

		MAX_WAIT_TIME(10*USECSPERSEC);

		FUNC(f, threadFunction);
		FUNC(g, writerThread);
		FUNC(fwait, pthread_cond_wait);

		TVAR(t1);
		TVAR(t2);
		TVAR(t3);

		WAIT_FOR_THREAD(t1, IN_FUNC(f), "t1");
		WAIT_FOR_DISTINCT_THREAD(t2, (t1), IN_FUNC(f), "t2");
		WAIT_FOR_DISTINCT_THREAD(t3, (t1, t2), IN_FUNC(g), "t3");

		RUN_UNTIL(NOT_BY(t2), HITS_PC(43, t1), "run t1 until after wait");

		RUN_UNTIL(NOT_BY(t1), HITS_PC(44, t2), "run !t1 until tail is -1");

		RUN_THREAD_UNTIL(t2, HITS_PC(45), "run t2 until 45");

		RUN_THREAD_UNTIL(t1, ENDS(), "run until ends"); // seg fault!

	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
