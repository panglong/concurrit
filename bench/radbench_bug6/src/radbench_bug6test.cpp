#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "MyScenario")

	TESTCASE() {

		FVAR(f_wlock, PR_RWLock_Wlock);
		FVAR(f_rlock, PR_RWLock_Rlock);
		FVAR(f_unlock, PR_RWLock_Unlock);

		MAX_WAIT_TIME(3*USECSPERSEC);

		WHILE_STAR {
			FORALL(t, PTRUE);
			RUN_UNTIL(BY(t), RETURNS(f_rlock) || RETURNS(f_wlock) || RETURNS(f_unlock) || ENDS(), __);
		}
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
