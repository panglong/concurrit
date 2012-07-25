#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {
		CALL_TEST(SearchInLargeSteps);
	}

	//================================//

	// GLOG_v=0 scripts/run_bench.sh pfscan -s -c
	// finfile: pqueue_get
	TEST(SearchInLargeSteps) {
		MAX_WAIT_TIME(3*USECSPERSEC);

		FUNC(fg, pqueue_get);
		FUNC(fp, pqueue_put);

		TVAR(t1);
		TVAR(t2);

		WAIT_FOR_DISTINCT_THREADS((t1, t2), IN_FUNC(fg), "T1 and T2");

		WHILE(!HAS_ENDED(t1) || !HAS_ENDED(t2)) {
			TVAR(t);
			CHOOSE_THREAD_BACKTRACK(t, (t1, t2), PTRUE);

			RUN_THREAD_THROUGH(t, HITS_PC() || ENTERS() || RETURNS() || ENDS(), "Run t until large step");
		}
	}

	//================================//

	// GLOG_v=0 scripts/run_bench.sh pfscan -s -c
	// finfile: pqueue_get
	TEST(ExactSchedule) {
		MAX_WAIT_TIME(3*USECSPERSEC);

		FUNC(fg, pqueue_get);
		FUNC(fp, pqueue_put);

		TVAR(t1);
		TVAR(t2);

		WAIT_FOR_DISTINCT_THREADS((t1, t2), IN_FUNC(fg), "T1 and T2");

		RUN_THREAD_UNTIL(t1, HITS_PC(42), "pqueue_get starts by T1");

		RUN_THREAD_THROUGH(t2, RETURNS(fg), "pqueue_get by T2");

		RUN_THREAD_THROUGH(t1, RETURNS(fg), "pqueue_get ends by T1");
	}

	//================================//

	// GLOG_v=0 scripts/run_bench.sh pfscan -s -c
	// finfile: pqueue_get
	TEST(SearchInFunc) {
		MAX_WAIT_TIME(3*USECSPERSEC);

		FUNC(fg, pqueue_get);
		FUNC(fp, pqueue_put);

		TVAR(t1);
		TVAR(t2);

		WAIT_FOR_DISTINCT_THREADS((t1, t2), IN_FUNC(fg), "T1 and T2");

		WHILE(!HAS_ENDED(t1) || !HAS_ENDED(t2)) {

			TVAR(t);
			CHOOSE_THREAD_BACKTRACK(t, (t1, t2), PTRUE);

			RUN_THREAD_THROUGH(t, READS() || WRITES() || CALLS() || HITS_PC() || ENDS(), "Run t until reads or writes memory");
		}
	}

	//================================//

	// GLOG_v=0 scripts/run_bench.sh pfscan -s -c
	// finfile: pqueue_get, pqueue_put
//	TEST(ModelCheckAll) {
//		MAX_WAIT_TIME(3*USECSPERSEC);
//
//		NDConcurrentSearch(PTRUE, READS() || WRITES() || ENDS());
//	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
