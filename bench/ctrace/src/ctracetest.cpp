#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "MyScenario")

	TESTCASE() {
		MAX_WAIT_TIME(3*USECSPERSEC);

//		TVAR(t_old);
		WHILE_STAR {
			FORALL(t, PTRUE); // TID != t_old
			RUN_UNTIL(BY(t), HITS_PC(42) || ENDS(), __); // t_old
		}
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
