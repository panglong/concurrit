#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {

		MAX_WAIT_TIME(USECSPERSEC);

		FUNC(fg, pqueue_get);
		FUNC(fp, pqueue_put);

		EXISTS(t1, IN_FUNC(fg), "T1");
		EXISTS(t2, NOT(t1) && IN_FUNC(fg), "T2");

		RUN_UNTIL(STEP(t1), AT_PC(42), __, "pqueue_get by T1");

		RUN_UNTIL(STEP(t2), RETURNS(fg), __, "pqueue_get by T2");

	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
