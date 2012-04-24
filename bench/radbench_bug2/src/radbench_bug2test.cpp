#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {

		FUNC(f_setcontextthread, JS_SetContextThread);
		FUNC(f_clearcontextthread, JS_ClearContextThread);
		FUNC(f_beginrequest, JS_BeginRequest);
		FUNC(f_gc, JS_GC);
		FUNC(f_endrequest, JS_EndRequest);

		MAX_WAIT_TIME(120*USECSPERSEC);

#define UNTIL_COND(f) 		(ENTERS(f) || RETURNS(f) || ENDS())
#define FULL_UNTIL_COND		(UNTIL_COND(f_setcontextthread) || UNTIL_COND(f_clearcontextthread) || UNTIL_COND(f_beginrequest) || UNTIL_COND(f_gc) || UNTIL_COND(f_endrequest))


#define READS_WRITES_OR_ENDS(t)		(READS(ANY_ADDR, t) || WRITES(ANY_ADDR, t) || ENDS(t))

		TVAR(t_main);
		RUN_UNTIL(PTRUE, ENTERS(f_gc), t_main, "Until GC");

		MAX_WAIT_TIME(10*USECSPERSEC);

		EXISTS(t, NOT(t_main), "Select t");
		WHILE_STAR {
			WHILE_STAR {
				RUN_UNTIL(BY(t), READS_WRITES_OR_ENDS(t), __, "Run t until ...");
			}

			RUN_UNTIL(PTRUE, READS_WRITES_OR_ENDS(t_main), __, "Run t_main until ...");
		}
}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
