#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()


//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

//	TESTCASE() {
//
//		MAX_WAIT_TIME(3*USECSPERSEC);
//
//		FUNC(dq, dequeue);
//
//		NDSequentialSearch();
//
//	}

//	TESTCASE() {
//
//		MAX_WAIT_TIME(3*USECSPERSEC);
//
//		FUNC(dq, dequeue);
//
//		FORALL(t1, IN_FUNC(dq), "T1");
//
////		RUN_UNTIL(PTRUE, AT_PC2(42, t1), __, "T1 starting");
//		RUN_UNTIL(PTRUE, AT_PC(42, t1), __, "T1 starting");
//
//		RUN_UNTIL(NOT(t1) && IN_FUNC(dq), RETURNS(dq), __, "Other threads");
//
//		RUN_UNTIL(STEP(t1), RETURNS(dq), __, "T1 returning");
//
//	}

	TESTCASE() {

		MAX_WAIT_TIME(10*USECSPERSEC);

		FUNC(dq, dequeue);

		FORALL(t1, IN_FUNC(dq), "T1");

		RUN_UNTIL(PTRUE, AT_PC(42, t1) || RETURNS(dq, t1), __, "T1 reaches 42");

		RUN_UNTIL(NOT(t1) && IN_FUNC(dq), RETURNS(dq), __, "Other threads");

		if(IN_FUNC(dq, t1)) {
			RUN_UNTIL(PTRUE, RETURNS(dq, t1), __, "T1 returning");
		} else {
			printf("t1 returned from dq!\n");
		}

	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
