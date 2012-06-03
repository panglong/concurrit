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

	FUNC(f_pr, PR_LocalTimeParameters);

	MAX_WAIT_TIME(0);

	WAIT_FOR_THREAD(t1, IN_FUNC(f_pr), "Select t1");
	WAIT_FOR_DISTINCT_THREAD(t2, IN_FUNC(f_pr), "Select t2");

	MAX_WAIT_TIME(USECSPERSEC);

	TVAR(t_old);

	WHILE(!HAS_ENDED(t1) && !HAS_ENDED(t2)) {

		IF(t_old->is_empty()) {
			SELECT_THREAD_BACKTRACK(t, PTRUE, "Select t");
		} ELSE {
			SELECT_THREAD_BACKTRACK(t, TID != t_old, "Select t");
		}

		RUN_THREAD_UNTIL(t, READS() || WRITES() || ENDS(), t_old, "Run t once");
	}
}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
