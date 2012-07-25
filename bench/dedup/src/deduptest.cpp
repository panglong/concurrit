#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()


//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {
		CALL_TEST(SearchInFunc);
	}

	//============================================================//

	// GLOG_v=0 scripts/run_bench.sh dedup -s -c
	// finfile: dequeue
	TEST(SearchInFunc) {

		MAX_WAIT_TIME(0);

		FUNC(f, dequeue);

		TVAR(t1);
		TVAR(t2);

		RUN_UNTIL(PTRUE, ENTERS(f), t1, "Run unless pc");
		ADDRINT qptr = ARG0(t1, f); // AuxState::Arg0->get(f, t1->tid());

		MAX_WAIT_TIME(15*USECSPERSEC);

		WAIT_FOR_THREAD(t2, ENTERS(f) && WITH_ARG0(f, qptr), ANY_THREAD - t1, "Select t2");
//		RUN_UNTIL(NOT_BY(t1), ENTERS(f) && WITH_ARG0(f, qptr), t2, "Select t2");

		MAX_WAIT_TIME(USECSPERSEC);

		WHILE(IN_FUNC(f, t1)->EvalState() || IN_FUNC(f, t2)->EvalState()) {

			TVAR(t);
			CHOOSE_THREAD_BACKTRACK(t, (t1, t2), "Select t");

			RUN_THREAD_THROUGH(t, READS() || WRITES() || CALLS() || RETURNS() || ENDS(), "Step t");
		}
	}
	//============================================================//

	// GLOG_v=0 scripts/run_bench.sh dedup -s -c
	// finfile: dequeue
	TEST(SearchInLargeSteps) {

		MAX_WAIT_TIME(0);

		FUNC(f, dequeue);

		TVAR(t1);
		TVAR(t2);

		RUN_UNTIL(PTRUE, ENTERS(f), t1, "Run unless pc");
		ADDRINT qptr = ARG0(t1, f); // AuxState::Arg0->get(f, t1->tid());

		MAX_WAIT_TIME(15*USECSPERSEC);

		WAIT_FOR_THREAD(t2, ENTERS(f) && WITH_ARG0(f, qptr), ANY_THREAD - t1, "Select t2");
		RUN_UNTIL(NOT_BY(t1), ENTERS(f) && WITH_ARG0(f, qptr), t2, "Select t2");

		MAX_WAIT_TIME(USECSPERSEC);

		WHILE(IN_FUNC(f, t1)->EvalState() || IN_FUNC(f, t2)->EvalState()) {

			TVAR(t);
			CHOOSE_THREAD_BACKTRACK(t, (t1, t2), "Select t");

			RUN_THREAD_THROUGH(t, HITS_PC() || ENTERS() || RETURNS() || ENDS(), "Step t");
		}
	}

	//============================================================//

	// robust
	// GLOG_v=0 scripts/run_bench.sh dedup -s -c
	// finfile: dequeue
	TEST(ExactSchedule) {

		MAX_WAIT_TIME(0);

		FUNC(f, dequeue);

		TVAR(t1);
		TVAR(t2);

		RUN_UNTIL(PTRUE, HITS_PC(42), t1, "Run unless pc");
		ADDRINT qptr = ARG0(t1, f); // AuxState::Arg0->get(f, t1->tid());

		MAX_WAIT_TIME(15*USECSPERSEC);

//		WAIT_FOR_DISTINCT_THREAD(t2, (t1), IN_FUNC(f) && WITH_ARG0(f, qptr), "Select t2");
		RUN_UNTIL(NOT_BY(t1), ENTERS(f) && WITH_ARG0(f, qptr), t2, "Select t2");

		RUN_THREAD_UNTIL(t2, RETURNS(f), "t2 until returns");

		RUN_THREAD_UNTIL(t1, RETURNS(f), "t1 until returns");
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
