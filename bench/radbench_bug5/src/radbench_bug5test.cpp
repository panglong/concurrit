#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "MyScenario")

	TESTCASE() {
		CALL_TEST(SearchInFunc);
	}

	//====================================================================

	// GLOG_v=0 scripts/run_bench.sh radbench_bug5 -s -c -r
	TEST(SearchInFunc) {

		FVAR(f_wait, PR_WaitCondVar);
		FVAR(f_interrupt, PR_Interrupt);

		MAX_WAIT_TIME(3*USECSPERSEC);

		TVAR(t1);
		TVAR(t2);

		WAIT_FOR_THREAD(t1, IN_FUNC(f_wait), "Waiting thread");
		WAIT_FOR_THREAD(t2, IN_FUNC(f_interrupt), ANY_THREAD - t1, "Interrupting thread");

		WHILE(!HAS_ENDED(t1) || !HAS_ENDED(t2)){
			TVAR(t);
			CHOOSE_THREAD_BACKTRACK(t, (t1, t2), PTRUE, "Select t");
			RUN_THREAD_THROUGH(t, READS() || WRITES() || CALLS() || RETURNS(), "Run t1");
		}
	}

	//====================================================================

	// GLOG_v=0 scripts/run_bench.sh radbench_bug5 -s -c -r
	TEST(SearchContextBounded) {

		FVAR(f_wait, PR_WaitCondVar);
		FVAR(f_interrupt, PR_Interrupt);

		MAX_WAIT_TIME(3*USECSPERSEC);

		TVAR(t1);
		TVAR(t2);

		WAIT_FOR_THREAD(t1, IN_FUNC(f_wait), "Waiting thread");

		WHILE_STAR {
			RUN_THREAD_THROUGH(t1, READS() || WRITES() || CALLS() || RETURNS(), "Run t1");
		}

		WAIT_FOR_THREAD(t2, IN_FUNC(f_interrupt), ANY_THREAD - t1, "Interrupting thread");

		RUN_THREAD_THROUGH(t2, RETURNS(f_interrupt), "Run t2");

		RUN_THREAD_THROUGH(t1, RETURNS(f_wait), "Run t1");
	}

	//====================================================================

	// GLOG_v=0 scripts/run_bench.sh radbench_bug5 -s -c -r
	TEST(ExactSchedule) {

		FVAR(f_wait, PR_WaitCondVar);
		FVAR(f_interrupt, PR_Interrupt);
		FVAR(f_pthread_wait, pthread_cond_wait);

		MAX_WAIT_TIME(3*USECSPERSEC);

		TVAR(t1);
		TVAR(t2);

		WAIT_FOR_THREAD(t1, ENTERS(f_wait), "Waiting thread");

		RUN_THREAD_UNTIL(t1, HITS_PC(42), "Run t1 until 42");

		WAIT_FOR_THREAD(t2, IN_FUNC(f_interrupt), ANY_THREAD - t1, "Interrupting thread");

		RUN_THREAD_THROUGH(t2, RETURNS(f_interrupt), "Run t2 until returns");

		RUN_THREAD_THROUGH(t1, RETURNS(f_wait), "Run t1 until returns");
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
