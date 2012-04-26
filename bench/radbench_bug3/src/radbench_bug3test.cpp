#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {

		FUNC(f_newcontext, JS_NewContext);
		FUNC(f_setcontextthread, JS_SetContextThread);
		FUNC(f_clearcontextthread, JS_ClearContextThread);
		FUNC(f_beginrequest, JS_BeginRequest);
		FUNC(f_gc, JS_GC);
		FUNC(f_endrequest, JS_EndRequest);

		MAX_WAIT_TIME(0);

#define READS_WRITES_OR_ENDS(t)		(READS(ANY_ADDR, t) || WRITES(ANY_ADDR, t) || ENDS(t))

		EXISTS(t1, PTRUE, "Select t1");
//		RUN_UNTIL(BY(t1), ENTERS(f_newcontext), __);

		EXISTS(t2, NOT(t1), "Select t2");
//		RUN_UNTIL(BY(t2), ENTERS(f_newcontext), __);

//		EXISTS(t3, NOT(t1) && NOT(t2), "Select t3");
//		RUN_UNTIL(BY(t3), ENTERS(f_newcontext), __);

		MAX_WAIT_TIME(3*USECSPERSEC);

//		WHILE_STAR {
//			RUN_UNTIL(BY(t1), READS_WRITES_OR_ENDS(t1), __, "Run t1 until ...");
//		}
//
//		WHILE_STAR {
//			RUN_UNTIL(BY(t2), READS_WRITES_OR_ENDS(t2), __, "Run t2 until ...");
//		}
//
//		RUN_UNTIL(BY(t1), ENDS(t1), __, "Run t1 until ends");

		WHILE_STAR {
			RUN_UNTIL(BY(t1), PTRUE, __, "Run t1 until ...");
		}

		WHILE_STAR {
			RUN_UNTIL(BY(t2), PTRUE, __, "Run t2 until ...");
		}

		RUN_UNTIL(BY(t1), ENDS(t1), __, "Run t1 until ends");
}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
