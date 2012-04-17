#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {

		MAX_WAIT_TIME(USECSPERSEC);

		FUNC(f, threadFunction);

		EXISTS(t1, IN_FUNC(f), "T1");
		EXISTS(t2, NOT(t1) && IN_FUNC(f), "T2");

		TVAR(t_old);
		WHILE_STAR {

			RUN_ONCE((STEP(t1) || STEP(t2)), __, "Interleaving");
		}

	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
