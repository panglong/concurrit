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

		TVAR(t_main);
		RUN_UNTIL(PTRUE, ENTERS(f_gc), t_main, "Until GC");

		MAX_WAIT_TIME(10*USECSPERSEC);

//		WHILE_STAR {
//			FORALL(t, NOT(t_main), "Select t");
//			RUN_UNTIL(BY(t), FULL_UNTIL_COND, __, "Run t until ...");
//
//			RUN_UNTIL(BY(t_main), ENTERS(ANY_FUNC) || RETURNS(ANY_FUNC) || ENDS(), __, "Run t_main until ...");
//		}


		EXISTS(t, NOT(t_main), "Select t");
		WHILE_AND_STAR(!HAS_ENDED(t)) {
			RUN_UNTIL(BY(t), ENTERS() || RETURNS() || ENDS(), __, "Run t until ...");
//			RUN_UNTIL(BY(t), READS() || WRITES() || ENDS(), __, "Run t until ...");
		}

		WHILE_AND_STAR(!HAS_ENDED(t_main)) {
			RUN_UNTIL(BY(t_main), ENTERS() || RETURNS() || ENDS(), __, "Run t_main until ...");
//			RUN_UNTIL(BY(t_main), READS() || WRITES() || ENDS(), __, "Run t_main until ...");
		}
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
