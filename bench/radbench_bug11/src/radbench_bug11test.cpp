#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "MyScenario")

	TESTCASE() {
		CALL_TEST(SearchInFunc);
	}

	TEST(SearchInFunc) {

		MAX_WAIT_TIME(10*USECSPERSEC);

		void* f_increment = ADDRINT2PTR(12345U); // ENTERS(f_add_delta)

		TVAR(t1);
		TVAR(t2);

		WAIT_FOR_THREAD(t1, ENTERS(f_increment), "Wait t1");
//              RUN_UNTIL(PTRUE, ENTERS(f_increment) && NOT_BY(t1), t2, "Wait t2");
		WAIT_FOR_DISTINCT_THREAD(t2, (t1), ENTERS(f_increment), "Wait t2");

		MAX_WAIT_TIME(3*USECSPERSEC);

		WHILE_STAR {
				TVAR(t);
				SELECT_THREAD_BACKTRACK(t, (t1, t2), PTRUE, "Select t");
				RUN_THREAD_THROUGH(t, HITS_PC() || RETURNS(f_increment), "Run t");
		}
	}


CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
