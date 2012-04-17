#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {

		MAX_WAIT_TIME(60*USECSPERSEC);

		FUNC(ls, localSearch);
		FUNC(lss, localSearchSub);

		EXISTS(tm, IN_FUNC(ls), "Main");

		//---------------------------------

		TVAR(t1);
		RUN_UNTIL(PTRUE, ENTERS(lss), t1, "until t1 enters");

		TVAR(t2);
		RUN_UNTIL(NOT(t1), ENTERS(lss), t2, "until t2 enters");

		TVAR(t3);
		RUN_UNTIL(NOT(t1) && NOT(t2), ENTERS(lss), t3, "until t3 enters");

		//---------------------------------

#define BRK1(t)	 (AT_PC2(42, t) || AT_PC2(45, t) || ENDS2(t))
#define BRK2(t)	 (AT_PC2(43, t) || AT_PC2(45, t) || ENDS2(t))

		WHILE_STAR {
			RUN_UNTIL(NOT(tm) && !AT_PC(44), BRK1(t1) && BRK1(t2) && BRK1(t3), __, "Run until 42");
			RUN_UNTIL(NOT(tm), BRK2(t1), __, "Run until 43");
		}


//		// do not use NOT(t3) since there is a barrier
//		RUN_UNTIL(NOT(tm), ENDS2(t1) && ENDS2(t2), __, "t1 and t2 ends");
//
//		//---------------------------------
//
//		RUN_UNTIL(STEP(tm), RETURNS(ls) || ENDS(), __, "Main returns");
//
//		//---------------------------------
//		MAX_WAIT_TIME(3*USECSPERSEC);
//
//		RUN_UNTIL(STEP(t3), RETURNS(lss) || ENDS(), __, "t3 returns");
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
