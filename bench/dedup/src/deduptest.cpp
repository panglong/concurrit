#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()


//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

//	TESTCASE() {
//
//		MAX_WAIT_TIME(10*USECSPERSEC);
//
//		FUNC(f, dequeue);
//
//		FORALL(t1, IN_FUNC(f), "T1");
//
//		ADDRINT qptr = AuxState::Arg0->get(f, t1->tid());
//
//		RUN_UNTIL(PTRUE, AT_PC(42, t1) || RETURNS(f, t1), __, "T1 reaches 42");
//
//		RUN_UNTIL(NOT(t1) && IN_FUNC(f) && WITH_ARG0(f, qptr), RETURNS(f), __, "Other threads");
//
//		if(IN_FUNC(f, t1)) {
//			RUN_UNTIL(PTRUE, RETURNS(f, t1), __, "T1 returning");
//		} else {
//			printf("t1 returned from dq!\n");
//		}
//
//	}


	TESTCASE() {

		MAX_WAIT_TIME(0);

		FUNC(f, dequeue);

		EXISTS(t1, IN_FUNC(f), "Select t1");
		ADDRINT qptr = AuxState::Arg0->get(f, t1->tid());

		MAX_WAIT_TIME(15*USECSPERSEC);

		EXISTS(t2, IN_FUNC(f) && NOT(t1) && WITH_ARG0(f, qptr), "Select t2");

		MAX_WAIT_TIME(USECSPERSEC);

		WHILE_STAR
		{
			FORALL(t, BY(t1) || BY(t2), "Select t");

			RUN_UNTIL(BY(t), READS() || WRITES() || HITS_PC(), "Step t");
		}

	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
