#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "MyScenario")

	TESTCASE() {
		CALL_TEST(ExactSchedule);
	}

	// GLOG_v=1 bench/radbench_bug8/src/runme.sh
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

	// GLOG_v=1 bench/radbench_bug8/src/runme.sh
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

	TEST(ExactSchedule) {

			MAX_WAIT_TIME(10*USECSPERSEC);

			void* f_increment = ADDRINT2PTR(12345U); // ENTERS(f_add_delta)

			TVAR(t1);
			TVAR(t2);

			WAIT_FOR_DISTINCT_THREADS((t1, t2), ENTERS(f_increment), "Wait t1, t2");

			MAX_WAIT_TIME(3*USECSPERSEC);

			RUN_THREAD_UNTIL(t1, HITS_PC(42), "Run t1 until 42");

			RUN_THREAD_UNTIL(t2, RETURNS(f_increment), "Run t2 until returns");

			RUN_THREAD_UNTIL(t1, RETURNS(f_increment), "Run t1 until returns");
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
