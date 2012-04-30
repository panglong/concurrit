#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {

		FUNC(f_newcontext, js_NewContext);
		FUNC(f_setcontextthread, js_SetContextThread);
		FUNC(f_clearcontextthread, js_ClearContextThread);
		FUNC(f_beginrequest, JS_BeginRequest);
		FUNC(f_gc, js_GC);
		FUNC(f_endrequest, JS_EndRequest);

		MAX_WAIT_TIME(0);

#define READS_WRITES_OR_ENDS(t)		(READS(ANY_ADDR, t) || WRITES(ANY_ADDR, t) || ENDS(t))

		EXISTS(t1, IN_FUNC(f_newcontext), "Select t1");

		EXISTS(t2, NOT(t1) && IN_FUNC(f_newcontext), "Select t2");

		MAX_WAIT_TIME(USECSPERSEC);

		WHILE_STAR {
			RUN_UNTIL(BY(t1), READS_WRITES_OR_ENDS(t1), __, "Run t1 until ...");
		}

		WHILE_STAR {
			RUN_UNTIL(BY(t2), READS_WRITES_OR_ENDS(t2), __, "Run t2 until ...");
		}

		RUN_UNTIL(BY(t1), ENDS(t1), __, "Run t1 until ends");
}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
