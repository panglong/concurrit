#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "MyScenario")

	TESTCASE() {

		WHILE_STAR {
			FORALL(t, ENTERS());
			RUN_UNTIL(BY(t), RETURNS() || ENDS(), __);
		}

	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
