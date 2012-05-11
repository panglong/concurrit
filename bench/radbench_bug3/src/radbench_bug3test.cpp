#include <stdio.h>

#include "concurrit.h"

#include "jsapi.h"
#include "jscntxt.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

//	TESTCASE() {
//
//		FUNC(f_newcontext, js_NewContext);
//		FUNC(f_setcontextthread, js_SetContextThread);
//		FUNC(f_clearcontextthread, js_ClearContextThread);
//		FUNC(f_beginrequest, JS_BeginRequest);
//		FUNC(f_gc, js_GC);
//		FUNC(f_endrequest, JS_EndRequest);
//
//		MAX_WAIT_TIME(0);
//
//#define READS_WRITES_OR_ENDS(x, t)		(READS(x, t) || WRITES(x, t) || ENDS(t) || ENTERS(ANY_FUNC, t) || RETURNS(ANY_FUNC, t))
//
//		EXISTS(t1, IN_FUNC(f_newcontext), "Select t1");
//		EXISTS(t2, NOT(t1) && IN_FUNC(f_newcontext), "Select t2");
//		EXISTS(t3, NOT(t1) && NOT(t2) && IN_FUNC(f_newcontext), "Select t3");
//
//		JSRuntime* rt = static_cast<JSRuntime*>(ADDRINT2PTR(AuxState::Arg0->get(f_newcontext, t1->tid())));
//		safe_assert(rt != NULL);
//
//		MAX_WAIT_TIME(3*USECSPERSEC);
//
//		WHILE_STAR {
//			FORALL(t, BY(t1) || BY(t2) || BY(t3));
//			RUN_UNTIL(BY(t), READS_WRITES_OR_ENDS(&rt->state, t), "Run t until ...");
//		}
//	}

//	TESTCASE() {
//
//		WHILE_STAR {
//			FORALL(t, PTRUE);
//			RUN_UNTIL(BY(t), PTRUE, __);
//		}
//	}


TESTCASE() {

		FUNC(f_newcontext, js_NewContext);
		FUNC(f_setcontextthread, js_SetContextThread);
		FUNC(f_clearcontextthread, js_ClearContextThread);
		FUNC(f_beginrequest, JS_BeginRequest);
		FUNC(f_gc, js_GC);
		FUNC(f_endrequest, JS_EndRequest);
		FUNC(f_destroycontext, js_DestroyContext);
		FUNC(f_prlock, PR_Lock);

		MAX_WAIT_TIME(0);

		EXISTS(t1, IN_FUNC(f_newcontext), "Select t1");
		EXISTS(t2, NOT(t1) && IN_FUNC(f_newcontext), "Select t2");
		EXISTS(t3, NOT(t1) && NOT(t2) && IN_FUNC(f_newcontext), "Select t3");

		JSRuntime* rt = static_cast<JSRuntime*>(ADDRINT2PTR(AuxState::Arg0->get(f_newcontext, t1->tid())));
		safe_assert(rt != NULL);
		void* x = &rt->state;

		RUN_UNTIL(BY(t1), ENTERS(f_destroycontext), __);
		RUN_UNTIL(BY(t2), ENTERS(f_destroycontext), __);

		MAX_WAIT_TIME(3*USECSPERSEC);

//		printf("Starting interleaving\n");

		WHILE_STAR {
			FORALL(t, BY(t1) || BY(t2) || BY(t3));
			RUN_UNTIL(BY(t), READS(x) || WRITES(x) || CALLS(f_prlock) || ENDS(), "Run t until ...");
		}
	}

//	TESTCASE() {
//
//		FUNC(f_newcontext, js_NewContext);
//		FUNC(f_setcontextthread, js_SetContextThread);
//		FUNC(f_clearcontextthread, js_ClearContextThread);
//		FUNC(f_beginrequest, JS_BeginRequest);
//		FUNC(f_gc, js_GC);
//		FUNC(f_endrequest, JS_EndRequest);
//		FUNC(f_destroycontext, js_DestroyContext);
//
//		MAX_WAIT_TIME(0);
//
//#define READS_WRITES_OR_ENDS(x, t)		(READS(x, t) || WRITES(x, t) || ENDS(t) || ENTERS(ANY_FUNC, t) || RETURNS(ANY_FUNC, t))
//
//		EXISTS(t1, IN_FUNC(f_newcontext), "Select t1");
//		EXISTS(t2, NOT(t1) && IN_FUNC(f_newcontext), "Select t2");
//		EXISTS(t3, NOT(t1) && NOT(t2) && IN_FUNC(f_newcontext), "Select t3");
//
//		RUN_UNTIL(BY(t1), ENTERS(f_destroycontext), __);
//		RUN_UNTIL(BY(t2), ENTERS(f_destroycontext), __);
//
//		MAX_WAIT_TIME(3*USECSPERSEC);
//
//		WHILE_STAR {
//			FORALL(t, BY(t1) || BY(t2) || BY(t3));
//			RUN_UNTIL(BY(t), PTRUE, "Run t until ...");
//		}
//	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
