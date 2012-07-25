#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "MyScenario")

	TESTCASE() {
		CALL_TEST(SearchInFuncLS3);
	}

	// GLOG_v=1 bench/radbench_bug15/src/runme.sh
	TEST(SearchInFuncLS2) {

		MAX_WAIT_TIME(10*USECSPERSEC);

		void* f_log = ADDRINT2PTR(12345U); // ENTERS(f_add_delta)
		void* f_insert = ADDRINT2PTR(12346U); // ENTERS(f_add_delta)

		TVAR(t1);
		TVAR(t2);

		WAIT_FOR_THREAD(t1, ENTERS(f_log), "Wait t1");
		WAIT_FOR_THREAD(t2, ENTERS(f_insert), ANY_THREAD - t1, "Wait t2");

		MAX_WAIT_TIME(3*USECSPERSEC);

		WHILE(IN_FUNC(f_log, t1)->EvalState() || IN_FUNC(f_insert, t2)->EvalState()) {
				TVAR(t);
				CHOOSE_THREAD_BACKTRACK(t, (t1, t2), PTRUE, "Select t");
				RUN_THREAD_THROUGH(t, HITS_PC() || RETURNS(f_log) || RETURNS(f_insert) || ENDS(), "Run t");
		}
	}

	TEST(SearchInFuncLS3) {

		MAX_WAIT_TIME(10*USECSPERSEC);

		void* f_log = ADDRINT2PTR(12345U); // ENTERS(f_add_delta)
		void* f_insert = ADDRINT2PTR(12346U); // ENTERS(f_add_delta)

		TVAR(t1);
		TVAR(t2);
		TVAR(t3);

		WAIT_FOR_DISTINCT_THREADS((t1, t2), ENTERS(f_log), "Wait t1, t2");
		WAIT_FOR_THREAD(t3, ENTERS(f_insert), ANY_THREAD - t1 - t2, "Wait t3");

		MAX_WAIT_TIME(3*USECSPERSEC);

		WHILE(IN_FUNC(f_log, t1)->EvalState() || IN_FUNC(f_log, t2)->EvalState() || IN_FUNC(f_insert, t3)->EvalState()) {
				TVAR(t);
				CHOOSE_THREAD_BACKTRACK(t, (t1, t2, t3), PTRUE, "Select t");
				RUN_THREAD_THROUGH(t, HITS_PC() || RETURNS(f_log) || RETURNS(f_insert) || ENDS(), "Run t");
		}
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
