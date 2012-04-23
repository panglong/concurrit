#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {

		FUNC(f_setcontext, JS_SetContextThread);
		FUNC(f_clearcontext, JS_ClearContextThread);
		FUNC(f_beginrequest, JS_BeginRequest);
		FUNC(f_gc, JS_GC);
		FUNC(f_endrequest, JS_EndRequest);

		MAX_WAIT_TIME(120*USECSPERSEC);

#define UNTIL_COND(f) 		(ENTERS(f) || RETURNS(f) || ENDS())
#define FULL_UNTIL_COND		UNTIL_COND(f_setcontext) || UNTIL_COND(f_clearcontext) || UNTIL_COND(f_beginrequest) || UNTIL_COND(f_gc) || UNTIL_COND(f_endrequest)

		WHILE_STAR {
			FORALL(t, PTRUE, "Select t");
			RUN_UNTIL(BY(t), FULL_UNTIL_COND, __, "Run t until ...");
		}
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
