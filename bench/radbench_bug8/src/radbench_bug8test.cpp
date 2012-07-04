#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "MyScenario")

	TESTCASE() {

		MAX_WAIT_TIME(10*USECSPERSEC);

		TVAR(t1);
		TVAR(t2);

		WAIT_FOR_THREAD(t1, PTRUE, "Wait t1");
		WAIT_FOR_DISTINCT_THREAD(t2, (t1), PTRUE, "Wait t2");

		MAX_WAIT_TIME(3*USECSPERSEC);

		WHILE_STAR {
			TVAR(t);
			SELECT_THREAD_BACKTRACK(t, (t1, t2), PTRUE, "Select t");
			RUN_THREAD_THROUGH(t, READS() || WRITES() || CALLS() || ENDS(), "Run t");
		}
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
