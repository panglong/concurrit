#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {
		CALL_TEST(SearchInFunc);
	}

	//============================================================//

	TEST(SearchAll) {

		FVAR(f_newcontext, js_NewContext);
		FVAR(f_setcontextthread, js_SetContextThread);
		FVAR(f_clearcontextthread, js_ClearContextThread);
		FVAR(f_beginrequest, JS_BeginRequest);
		FVAR(f_gc, js_GC);
		FVAR(f_endrequest, JS_EndRequest);

			MAX_WAIT_TIME(3*USECSPERSEC);

	//		TVAR(t);
	//		RUN_UNTIL(!IN_FVAR(f_beginrequest), ENTERS(f_setcontextthread), t, "Select t");
	//
	//		TVAR(t_main);
	//		RUN_UNTIL(NOT_BY(t), ENTERS(f_beginrequest), t_main, "Select t_main");
	//
	//		MAX_WAIT_TIME(10*USECSPERSEC);
	//
	//		WHILE_STAR {
	//			WHILE_STAR {
	//				RUN_UNTIL(BY(t), READS_WRITES_OR_ENDS(t), __, "Run t until ...");
	//			}
	//
	//			RUN_UNTIL(BY(t_main), READS_WRITES_OR_ENDS(t_main), __, "Run t_main until ...");
	//		}

			UNTIL_ALL_END {
				TVAR(t);
				FORALL(t, PTRUE);
				RUN_THREAD_THROUGH(t, READS() || WRITES() || ENDS(), "Run t until");
			}
	}

	//============================================================//

	// GLOG_v=1 scripts/run_bench.sh radbench_bug2 -s -c -r
	TEST(SearchInFunc) {

//		What I suspect is happening is that one thread is calling JS_GC while a second
//		is calling JS_EndRequest and JS_ClearContextThread (in returning a context to
//		the pool). The call to JS_GC will block until JS_EndRequest finishes.. JS_GC
//		then resumes.. but while JS_GC is running JS_ClearContextThread also runs (no
//		locking is done in this?), modifying the value of acx->thread as the code above
//		runs. acx->thread becomes NULL just before it gets dereferenced and the
//		application exits.

		FVAR(f_clearcontextthread, JS_ClearContextThread);
		FVAR(f_gc, JS_GC);

		MAX_WAIT_TIME(0);

		TVAR(t1);
		WAIT_FOR_THREAD(t1, "Select t1 main");

		RUN_THREAD_UNTIL(t1, ENTERS(f_gc), "Run t1 until gc");

		TVAR(t2);
		WAIT_FOR_THREAD(t2, ANY_THREAD - t1, "Select t2");

		RUN_THREAD_UNTIL(t2, ENTERS(f_clearcontextthread), "Run until clear context");

		MAX_WAIT_TIME(USECSPERSEC);

		WHILE(!HAS_ENDED(t1) || !HAS_ENDED(t2)) {

			TVAR(t);
			CHOOSE_THREAD_BACKTRACK(t, (t1, t2), PTRUE);

			RUN_THREAD_THROUGH(t, READS() || WRITES() || ENDS(), "Run t until");
		}
	}

	// GLOG_v=1 scripts/run_bench.sh radbench_bug2 -s -c -r
	TEST(SearchTwoContextBoundedInFunc) {

//		What I suspect is happening is that one thread is calling JS_GC while a second
//		is calling JS_EndRequest and JS_ClearContextThread (in returning a context to
//		the pool). The call to JS_GC will block until JS_EndRequest finishes.. JS_GC
//		then resumes.. but while JS_GC is running JS_ClearContextThread also runs (no
//		locking is done in this?), modifying the value of acx->thread as the code above
//		runs. acx->thread becomes NULL just before it gets dereferenced and the
//		application exits.

		FVAR(f_clearcontextthread, JS_ClearContextThread);
		FVAR(f_gc, JS_GC);

		MAX_WAIT_TIME(0);

		TVAR(t1);
		WAIT_FOR_THREAD(t1, "Select t1 main");

		RUN_THREAD_UNTIL(t1, ENTERS(f_gc), "Run t1 until gc");

		TVAR(t2);
		WAIT_FOR_THREAD(t2, ANY_THREAD - t1, "Select t2");

		RUN_THREAD_UNTIL(t2, ENTERS(f_clearcontextthread), "Run until clear context");

		MAX_WAIT_TIME(USECSPERSEC);

		TVAR(t);
		CHOOSE_THREAD_BACKTRACK(t, (t1, t2), PTRUE, "Select t");

		WHILE_STAR {
			RUN_THREAD_THROUGH(t, READS() || WRITES() || ENDS(), "Run t until");
		}

		t = (t == t1) ? t2 : t1;

		RUN_THREAD_THROUGH(t, ENDS(), "Run t");
	}

	// GLOG_v=1 scripts/run_bench.sh radbench_bug2 -s -c -r
	TEST(SearchThreeContextBoundedInFunc) {

//		What I suspect is happening is that one thread is calling JS_GC while a second
//		is calling JS_EndRequest and JS_ClearContextThread (in returning a context to
//		the pool). The call to JS_GC will block until JS_EndRequest finishes.. JS_GC
//		then resumes.. but while JS_GC is running JS_ClearContextThread also runs (no
//		locking is done in this?), modifying the value of acx->thread as the code above
//		runs. acx->thread becomes NULL just before it gets dereferenced and the
//		application exits.

		FVAR(f_clearcontextthread, JS_ClearContextThread);
		FVAR(f_gc, JS_GC);

		MAX_WAIT_TIME(0);

		TVAR(t1);
		WAIT_FOR_THREAD(t1, "Select t1 main");

		RUN_THREAD_UNTIL(t1, ENTERS(f_gc), "Run t1 until gc");

		TVAR(t2);
		WAIT_FOR_THREAD(t2, ANY_THREAD - t1, "Select t2");

		RUN_THREAD_UNTIL(t2, ENTERS(f_clearcontextthread), "Run until clear context");

		MAX_WAIT_TIME(USECSPERSEC);

		TVAR(t);
		CHOOSE_THREAD_BACKTRACK(t, (t1, t2), PTRUE, "Select t");

		WHILE_STAR {
			RUN_THREAD_THROUGH(t, READS() || WRITES() || ENDS(), "Run t until");
		}

		t = (t == t1) ? t2 : t1;

		WHILE_STAR {
			RUN_THREAD_THROUGH(t, READS() || WRITES() || ENDS(), "Run t until");
		}

		t = (t == t1) ? t2 : t1;

		RUN_THREAD_THROUGH(t, ENDS(), "Run t");
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
