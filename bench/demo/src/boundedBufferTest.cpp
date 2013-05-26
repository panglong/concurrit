#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(BBScenario, "Bounded buffer scenario")

	TESTCASE() {
		CALL_TEST(SearchSequential);
	}

	//============================================================//

	TEST(SearchSequential) {

		MAX_WAIT_TIME(10*USECSPERSEC);

		FUNCT(bounded_buf_get);
		FUNCT(bounded_buf_put);
		FUNCT(producer_routine);
		FUNCT(consumer_routine);

		TVAR(P1);
		TVAR(P2);
		TVAR(C1);
		TVAR(C2);

		WAIT_FOR_DISTINCT_THREADS((P1, P2), ENTERS(producer_routine), "Wait for 2 producers.");

		WAIT_FOR_DISTINCT_THREADS((C1, C2), ENTERS(consumer_routine), "Wait for 2 consumers.");

		WHILE_STAR {
			TVAR(t);

			CHOOSE_THREAD_BACKTRACK(t, (P1, P2, C1, C2), PTRUE, "Select a thread to execute.");

			RUN_THREAD_THROUGH(t, ENDS(), "Run t until ends.");
		}
	}


	//============================================================//

	// GLOG_v=0 scripts/run_bench.sh bbuf -s -c
	// finfile: bounded_buf_get bounded_buf_put
	TEST(SearchAll) {

		MAX_WAIT_TIME(3*USECSPERSEC);

		WHILE_STAR {
			TVAR(t);
			// waits for any thread in the SUT
			CHOOSE_THREAD_BACKTRACK(t, (), PTRUE, "Select");
			RUN_THREAD_THROUGH(t, READS() || WRITES() || CALLS() || HITS_PC() || ENDS(), "Run t");
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

		WAIT_FOR_DISTINCT_THREADS((t1, t2), IN_FUNC(f_get));

		WAIT_FOR_DISTINCT_THREADS((t3, t4), IN_FUNC(f_put), ANY_THREAD - t1 - t2);

		MAX_WAIT_TIME(USECSPERSEC);

		WHILE(!HAVE_ENDED(t1, t2, t3, t4)) {

			TVAR(t);
			CHOOSE_THREAD_BACKTRACK(t, (t1, t2, t3, t4), PTRUE, "Select t");
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

		WAIT_FOR_DISTINCT_THREADS((t1, t2), ENTERS(f_get));

		MAX_WAIT_TIME(USECSPERSEC);

		WHILE(!HAVE_ENDED(t1, t2)) {

			TVAR(t);
			CHOOSE_THREAD_BACKTRACK(t, (t1, t2), PTRUE, "Select t");
			RUN_THREAD_THROUGH(t, READS() || WRITES() || CALLS() || HITS_PC() || ENDS(), "Run t until...");
		}
	}

CONCURRIT_END_TEST(BBScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
