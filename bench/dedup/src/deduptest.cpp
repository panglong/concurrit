#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()


//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

//	TESTCASE() {
//
//		MAX_WAIT_TIME(10*USECSPERSEC);
//
//		FUNC(f, dequeue);
//
//		FORALL(t1, IN_FUNC(f), "T1");
//
//		ADDRINT qptr = AuxState::Arg0->get(f, t1->tid());
//
//		RUN_UNTIL(PTRUE, AT_PC(42, t1) || RETURNS(f, t1), __, "T1 reaches 42");
//
//		RUN_UNTIL(NOT(t1) && IN_FUNC(f) && WITH_ARG0(f, qptr), RETURNS(f), __, "Other threads");
//
//		if(IN_FUNC(f, t1)) {
//			RUN_UNTIL(PTRUE, RETURNS(f, t1), __, "T1 returning");
//		} else {
//			printf("t1 returned from dq!\n");
//		}
//
//	}

	TESTCASE() {
		CALL_TEST(Final4);
	}

	//============================================================//

	// GLOG_v=0 scripts/run_bench.sh dedup -s -c
	// finfile: dequeue
	TEST(Modelcheck) {

		MAX_WAIT_TIME(0);

		FUNC(f, dequeue);

		FORALL(t1, IN_FUNC(f), "Select t1");
		ADDRINT qptr = ARG0(t1, f); // AuxState::Arg0->get(f, t1->tid());

		MAX_WAIT_TIME(15*USECSPERSEC);

		FORALL(t2, TID != t1 && IN_FUNC(f) && WITH_ARG0(f, qptr), "Select t2");

		MAX_WAIT_TIME(USECSPERSEC);

		WHILE(!HAS_ENDED(t1) && !HAS_ENDED(t2)) {

			SELECT_THREAD_BACKTRACK(t, PTRUE, "Select t");

			RUN_THREAD_ONCE(t, "Step t");
		}
	}

	//============================================================//

	// GLOG_v=0 scripts/run_bench.sh dedup -s -c
	// finfile: dequeue
	TEST(Final0) {

		MAX_WAIT_TIME(0);

		FUNC(f, dequeue);

		WAIT_FOR_THREAD(t1, IN_FUNC(f), "Select t1");
		ADDRINT qptr = ARG0(t1, f); // AuxState::Arg0->get(f, t1->tid());

		MAX_WAIT_TIME(15*USECSPERSEC);

		WAIT_FOR_DISTINCT_THREAD(t2, IN_FUNC(f) && WITH_ARG0(f, qptr), "Select t2");

		MAX_WAIT_TIME(USECSPERSEC);

		WHILE(!HAS_ENDED(t1) && !HAS_ENDED(t2)) {

			SELECT_THREAD_BACKTRACK(t, PTRUE, "Select t");

			RUN_THREAD_UNTIL(t, READS() || WRITES() || ENDS(), "Step t");
		}
	}

	//============================================================//

	// GLOG_v=0 scripts/run_bench.sh dedup -s -c
	// finfile: dequeue
	TEST(Final1) {

		MAX_WAIT_TIME(0);

		FUNC(f, dequeue);

		WAIT_FOR_THREAD(t1, IN_FUNC(f), "Select t1");
		ADDRINT qptr = ARG0(t1, f); // AuxState::Arg0->get(f, t1->tid());

		MAX_WAIT_TIME(15*USECSPERSEC);

		WAIT_FOR_DISTINCT_THREAD(t2, IN_FUNC(f) && WITH_ARG0(f, qptr), "Select t2");

		MAX_WAIT_TIME(USECSPERSEC);

		WHILE(!HAS_ENDED(t1) && !HAS_ENDED(t2)) {

			SELECT_THREAD_BACKTRACK(t, PTRUE, "Select t");

			RUN_THREAD_UNTIL(t, READS() || WRITES() || HITS_PC() || ENDS(), "Step t");
		}
	}

	//============================================================//

	// GLOG_v=0 scripts/run_bench.sh dedup -s -c
	// finfile: dequeue
	TEST(Final2) {

		MAX_WAIT_TIME(0);

		FUNC(f, dequeue);

		WAIT_FOR_THREAD(t1, IN_FUNC(f), "Select t1");
		ADDRINT qptr = ARG0(t1, f); // AuxState::Arg0->get(f, t1->tid());

		MAX_WAIT_TIME(15*USECSPERSEC);

		WAIT_FOR_DISTINCT_THREAD(t2, IN_FUNC(f) && WITH_ARG0(f, qptr), "Select t2");

		MAX_WAIT_TIME(USECSPERSEC);

		WHILE(!HAS_ENDED(t1) && !HAS_ENDED(t2)) {

			SELECT_THREAD_BACKTRACK(t, PTRUE, "Select t");

			RUN_THREAD_UNTIL(t, HITS_PC() || ENDS(), "Step t");
		}
	}

	//============================================================//

	// GLOG_v=0 scripts/run_bench.sh dedup -s -c
	// finfile: dequeue
	TEST(Final3) {

		MAX_WAIT_TIME(0);

		FUNC(f, dequeue);

		WAIT_FOR_THREAD(t1, IN_FUNC(f), "Select t1");
		ADDRINT qptr = ARG0(t1, f); // AuxState::Arg0->get(f, t1->tid());

		RUN_ANY_THREAD_UNLESS(HITS_PC(42, t1), "Run unless pc");

		MAX_WAIT_TIME(15*USECSPERSEC);

		WAIT_FOR_DISTINCT_THREAD(t2, IN_FUNC(f) && WITH_ARG0(f, qptr), "Select t2");

		RUN_THREAD_UNTIL(t2, ENDS(), "t2 until ends");

		RUN_THREAD_UNTIL(t1, RETURNS(f), "t1 until ends");
	}

	//============================================================//

	// robust
	// GLOG_v=0 scripts/run_bench.sh dedup -s -c
	// finfile: dequeue
	TEST(Final4) {

		MAX_WAIT_TIME(0);

		FUNC(f, dequeue);

		TVAR(t1);
		RUN_ANY_THREAD_UNLESS(HITS_PC(42), t1, "Run unless pc");
		ADDRINT qptr = ARG0(t1, f); // AuxState::Arg0->get(f, t1->tid());

		MAX_WAIT_TIME(15*USECSPERSEC);

		WAIT_FOR_DISTINCT_THREAD(t2, IN_FUNC(f) && WITH_ARG0(f, qptr), "Select t2");

		RUN_ANY_THREAD_BUT_UNTIL(t1, ENDS(), t2, "t2 until ends");

		RUN_THREAD_UNTIL(t1, RETURNS(f), "t1 until ends");
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
