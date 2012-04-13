#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {

		MAX_WAIT_TIME(5*USECSPERSEC);

		FUNC(f, threadFunction);

		EXISTS(t1, IN_FUNC(f), "T1");
		EXISTS(t2, NOT(t1) && IN_FUNC(f), "T2");

		RUN_UNTIL(PTRUE, (RETURNS(f)), __, "Interleaving");


//		WHILE_STAR {
//			RUN_ONCE((t1 || t2), __, "Interleaving");
//		}

	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
