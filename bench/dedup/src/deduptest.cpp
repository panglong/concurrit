#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {

		MAX_WAIT_TIME(3*USECSPERSEC);

		FUNC(dq, dequeue);

		EXISTS(t1, IN_FUNC(dq), "T1");

//		RUN_UNTIL(PTRUE, AT_PC2(42, t1), __, "T1 starting");
		RUN_UNTIL(STEP(t1), AT_PC(42), __, "T1 starting");

		RUN_UNTIL(NOT(t1) && IN_FUNC(dq), RETURNS(dq), __, "Other threads");

		RUN_UNTIL(STEP(t1), RETURNS(dq), __, "T1 returning");

	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
