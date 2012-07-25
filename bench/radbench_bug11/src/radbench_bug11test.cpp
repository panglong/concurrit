#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "MyScenario")

	TESTCASE() {
		CALL_TEST(SearchInFunc2_0);
	}

	// GLOG_v=1 bench/radbench_bug11/src/runme.sh
	TEST(SearchInFunc3) {

			MAX_WAIT_TIME(10*USECSPERSEC);

			void* f_increment = ADDRINT2PTR(12345U); // ENTERS(f_add_delta)

			TVAR(t1);
			TVAR(t2);
			TVAR(t3);

			WAIT_FOR_DISTINCT_THREADS((t1, t2, t3), ENTERS(f_increment), "Wait t1, t2, and t3");

			MAX_WAIT_TIME(3*USECSPERSEC);

			WHILE_STAR {
					TVAR(t);
					CHOOSE_THREAD_BACKTRACK(t, (t1, t2, t3), PTRUE, "Select t");
					RUN_THREAD_THROUGH(t, HITS_PC() || RETURNS(f_increment), "Run t");
			}
	}

	// GLOG_v=1 bench/radbench_bug11/src/runme.sh
	TEST(SearchInFunc2) {

			MAX_WAIT_TIME(10*USECSPERSEC);

			void* f_increment = ADDRINT2PTR(12345U); // ENTERS(f_add_delta)

			TVAR(t1);
			TVAR(t2);

			WAIT_FOR_DISTINCT_THREADS((t1, t2), ENTERS(f_increment), "Wait t1, t2");

			MAX_WAIT_TIME(3*USECSPERSEC);

			WHILE_STAR {
					TVAR(t);
					CHOOSE_THREAD_BACKTRACK(t, (t1, t2), PTRUE, "Select t");
					RUN_THREAD_THROUGH(t, HITS_PC() || RETURNS(f_increment), "Run t");
			}
	}

	// GLOG_v=1 bench/radbench_bug11/src/runme.sh
	TEST(SearchInFunc2_0) {

			MAX_WAIT_TIME(10*USECSPERSEC);

			void* f_increment = ADDRINT2PTR(12345U); // ENTERS(f_add_delta)
			void* f_decrement = ADDRINT2PTR(12346U); // ENTERS(f_add_delta)

			TVAR(t1);
			TVAR(t2);

			WAIT_FOR_THREAD(t1, ENTERS(f_increment), "Wait t1");
			RUN_UNTIL(NOT_BY(t1), ENTERS(f_decrement), t2, "Wait t2");

			MAX_WAIT_TIME(3*USECSPERSEC);

			WHILE_STAR {
//			WHILE(IN_FUNC(f_increment, t1)->EvalState() || IN_FUNC(f_decrement, t2)->EvalState()) {
					TVAR(t);
					CHOOSE_THREAD_BACKTRACK(t, (t1, t2), PTRUE, "Select t");
					RUN_THREAD_THROUGH(t, HITS_PC() || RETURNS(f_increment) || RETURNS(f_decrement) || ENDS(), "Run t");
			}
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
