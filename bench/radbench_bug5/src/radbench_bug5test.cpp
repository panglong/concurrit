#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "MyScenario")

//	TESTCASE() {
//
//		MAX_WAIT_TIME(3*USECSPERSEC);
//		NDConcurrentSearch(PTRUE, READS() || WRITES() || ENDS());
//
//
//	}

//	TESTCASE() {
//
//		FVAR(f_wait, PR_WaitCondVar);
//		FVAR(f_interrupt, PR_Interrupt);
//
//		EXISTS(t1, IN_FUNC(f_wait), "Waiting thread");
//		EXISTS(t2, IN_FUNC(f_interrupt), "Interrupting thread");
//
//		FORALL(t, BY(t1) || BY(t2), "Select t");
//		WHILE_STAR {
//			RUN_UNTIL(BY(t), READS() || WRITES() || ENDS(), "Run tt");
//		}
//
//		EXISTS(tt, (BY(t1) || BY(t2)) && (NOT(t)), "Select tt");
//		WHILE_STAR {
//			RUN_UNTIL(BY(tt), READS() || WRITES() || ENDS(), "Run tt");
//		}
//
//		WHILE_STAR {
//			RUN_UNTIL(BY(t), READS() || WRITES() || ENDS(), "Run tt");
//		}
//
//
//	}

	TESTCASE() {

		FVAR(f_wait, PR_WaitCondVar);
		FVAR(f_interrupt, PR_Interrupt);

		MAX_WAIT_TIME(3*USECSPERSEC);

		EXISTS(t1, IN_FUNC(f_wait), "Waiting thread");
		EXISTS(t2, NOT(t1) && IN_FUNC(f_interrupt), "Interrupting thread");

		WHILE_NDSTAR {
			RUN_UNTIL(BY(t1), READS() || WRITES() || RETURNS(), "Run t1");
		}

		RUN_UNTIL(BY(t2), RETURNS(), "Run t2");

		RUN_UNTIL(BY(t1), RETURNS() || ENDS(), "Run t1");
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
