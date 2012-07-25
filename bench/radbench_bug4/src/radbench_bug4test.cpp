#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	// not robust, if not trigger the bug at the first execution
	// GLOG_v=0 scripts/run_bench.sh radbench_bug4 -s -c -r
	// finfile: PR_LocalTimeParameters MT_safe_localtime
	TESTCASE() {
		CALL_TEST(SearchInFunc);
	}

	//====================================================================

	// GLOG_v=0 scripts/run_bench.sh radbench_bug4 -s -c -r
	TEST(SearchInFunc) {
		FUNC(f_mt, MT_safe_localtime);
		FUNC(f_pr, PR_LocalTimeParameters);

		MAX_WAIT_TIME(0);

		TVAR(t1);
		TVAR(t2);

		WAIT_FOR_DISTINCT_THREADS((t1, t2), ENTERS(f_mt), "Select t1, t2");

		MAX_WAIT_TIME(USECSPERSEC);

		WHILE_STAR {
//		WHILE(!HAS_ENDED(t1) && !HAS_ENDED(t2)) {
			TVAR(t);
			CHOOSE_THREAD_BACKTRACK(t, (t1, t2), PTRUE, "Select t");
			RUN_THREAD_THROUGH(t, READS() || WRITES() || CALLS() || ENDS(), "Run t once");
		}
	}

	//====================================================================

	// GLOG_v=0 scripts/run_bench.sh radbench_bug4 -s -c -r
	TEST(SearchTwoContext) {
		FUNC(f_mt, MT_safe_localtime);
		FUNC(f_pr, PR_LocalTimeParameters);

		MAX_WAIT_TIME(0);

		TVAR(t1);
		TVAR(t2);

		WAIT_FOR_DISTINCT_THREADS((t1, t2), ENTERS(f_mt), "Select t1, t2");

		MAX_WAIT_TIME(USECSPERSEC);

		TVAR(t);
		CHOOSE_THREAD_BACKTRACK(t, (t1, t2), PTRUE, "Select t");

		WHILE_STAR {
			RUN_THREAD_THROUGH(t, READS() || WRITES() || CALLS() || ENDS(), "Run t until");
		}

		t = (t == t1) ? t2 : t1;

		RUN_THREAD_THROUGH(t, ENDS(), "Run t until ends");
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
