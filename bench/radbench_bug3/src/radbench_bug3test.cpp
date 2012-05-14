#include <stdio.h>

#include "concurrit.h"

#include "jsapi.h"
#include "jscntxt.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

//	TESTCASE() { // 6 schedules
//
//		MAX_WAIT_TIME(0);
//
//		FUNC(f_newcontext, JS_NewContext);
//		FUNC(f_beginrequest, JS_BeginRequest);
//		FUNC(f_gc, JS_GC);
//		FUNC(f_destroycontext, JS_DestroyContext);
//
//		EXISTS(t1, IN_FUNC(f_newcontext), "Select t1");
//		EXISTS(t2, (TID != t1) && IN_FUNC(f_newcontext), "Select t2");
//		EXISTS(t3, (TID != t1) && (TID != t2) && IN_FUNC(f_newcontext), "Select t3");
//
//		WHILE_STAR {
//			FORALL(t, (TID == t1) || (TID == t2) || (TID == t3));
//			RUN_UNTIL(TID == t, ENDS(), __);
//		}
//	}

//************************************************************************************

//	TESTCASE() { // 50 schedules, 5 minutes
//
//		MAX_WAIT_TIME(0);
//
//		FUNC(f_newcontext, JS_NewContext);
//		FUNC(f_beginrequest, JS_BeginRequest);
//		FUNC(f_gc, JS_GC);
//		FUNC(f_destroycontext, JS_DestroyContext);
//
//		EXISTS(t1, IN_FUNC(f_newcontext), "Select t1");
//		EXISTS(t2, (TID != t1) && IN_FUNC(f_newcontext), "Select t2");
//		EXISTS(t3, (TID != t1) && (TID != t2) && IN_FUNC(f_newcontext), "Select t3");
//
//		WHILE_STAR {
//			FORALL(t, (TID == t1) || (TID == t2) || (TID == t3));
//			RUN_UNTIL(TID == t, RETURNS(f_newcontext) || RETURNS(f_beginrequest) || RETURNS(f_destroycontext), "Run until returns...");
//		}
//	}

//************************************************************************************

	TESTCASE() {

		MAX_WAIT_TIME(0);

		FUNC(f_newcontext, JS_NewContext);
		FUNC(f_beginrequest, JS_BeginRequest);
		FUNC(f_gc, js_GC);
		FUNC(f_GC, JS_GC);
		FUNC(f_destroycontext, JS_DestroyContext);

//		Now suppose there are 3 threads, A, B, C.
		EXISTS(t1, IN_FUNC(f_newcontext), "Select t1");
		EXISTS(t2, (TID != t1) && IN_FUNC(f_newcontext), "Select t2");
		EXISTS(t3, (TID != t1) && (TID != t2) && IN_FUNC(f_newcontext), "Select t3");

		JSRuntime* rt = static_cast<JSRuntime*>(ADDRINT2PTR(AuxState::Arg0->get(f_newcontext, t1->tid())));
		safe_assert(rt != NULL);
		void* RT_STATE = &(rt->state);
//		printf("rtstate: %lx\n", RT_STATE);
		void* RT_GCLOCK = ADDRINT2PTR(PTR2ADDRINT(&(rt->gcLock)) + 0x10L);
//		printf("gclock: %lx\n", RT_GCLOCK);
		void* RT_GCTHREAD = ADDRINT2PTR(PTR2ADDRINT(&(rt->gcThread)) + 0x10L);
//		printf("gcthread: %lx\n", RT_GCTHREAD);

		MAX_WAIT_TIME(10*USECSPERSEC);

		//======================================================

		RUN_UNTIL(BY(t1), ENTERS(f_destroycontext), __);
		RUN_UNTIL(BY(t2), ENTERS(f_destroycontext), __);

		MAX_WAIT_TIME(3*USECSPERSEC);

		WHILE_STAR {
			FORALL(t, (TID == t1) || (TID == t2) || (TID == t3));

//			RUN_UNTIL(TID == t, PTRUE, "Run until true...");

//			// 5 schedules, NO BUG!
//			RUN_UNTIL(TID == t, CALLS() || ENDS(), "Run until calls...");

//			RUN_UNTIL(TID == t, ACCESSES() || ENDS(), "Run until accesses...");

//			RUN_UNTIL(TID == t, CALLS() || ACCESSES() || ENDS(), "Run until accesses...");

			// 79 schedules, 6 minutes, BUG!
			RUN_UNTIL(TID == t, ACCESSES(RT_STATE) || ACCESSES(RT_GCTHREAD) || ACCESSES(RT_GCLOCK) || ENTERS() || ENDS(), "Run until accesses...");
		}
	}


//************************************************************************************

//	TESTCASE() {
//
//		MAX_WAIT_TIME(0);
//
//		FUNC(f_newcontext, JS_NewContext);
//		FUNC(f_beginrequest, JS_BeginRequest);
//		FUNC(f_gc, js_GC);
//		FUNC(f_GC, JS_GC);
//		FUNC(f_destroycontext, JS_DestroyContext);
//
////		Now suppose there are 3 threads, A, B, C.
//		EXISTS(t1, IN_FUNC(f_newcontext), "Select t1");
//		EXISTS(t2, (TID != t1) && IN_FUNC(f_newcontext), "Select t2");
//		EXISTS(t3, (TID != t1) && (TID != t2) && IN_FUNC(f_newcontext), "Select t3");
//
//		JSRuntime* rt = static_cast<JSRuntime*>(ADDRINT2PTR(AuxState::Arg0->get(f_newcontext, t1->tid())));
//		safe_assert(rt != NULL);
//		void* RT_STATE = &(rt->state);
////		printf("rtstate: %lx\n", RT_STATE);
//		void* RT_GCLOCK = ADDRINT2PTR(PTR2ADDRINT(&(rt->gcLock)) + 0x10L);
////		printf("gclock: %lx\n", RT_GCLOCK);
//		void* RT_GCTHREAD = ADDRINT2PTR(PTR2ADDRINT(&(rt->gcThread)) + 0x10L);
////		printf("gcthread: %lx\n", RT_GCTHREAD);
//
//		MAX_WAIT_TIME(20*USECSPERSEC);
//
//		//======================================================
//
////		Threads A and B calls js_DestroyContext and thread C calls js_NewContext.
//		RUN_UNTIL(BY(t1), ENTERS(f_destroycontext), __);
//		RUN_UNTIL(BY(t2), ENTERS(f_destroycontext), __);
//
//		//======================================================
//
////		First thread A removes its context from the runtime list. That context is not
////		the last one so thread does not touch rt->state and eventually calls js_GC. The
////		latter skips the above check and tries to to take the GC lock.
//		RUN_UNTIL(BY(t1), IN_FUNC(f_gc) && READS(RT_STATE), "Run TA until js_GC");
//
////		Before this moment the thread B takes the lock, removes its context from the
////		runtime list, discovers that it is the last, sets rt->state to LANDING, runs
////		the-last-context-cleanup, runs the GC and then sets rt->state to DOWN.
//		RUN_UNTIL(BY(t2), ENDS(), "Run TB ends");
//
////		At this stage the thread A gets the GC lock, setup itself as the thread that
////		runs the GC and releases the GC lock to proceed with the GC when rt->state is
////		DOWN.
//		RUN_UNTIL(BY(t1), IN_FUNC(f_gc) && WRITES(RT_GCTHREAD), "Run TA until writes to rt->gcThread");
//
////		Now the thread C enters the picture. It discovers under the GC lock in
////		js_NewContext that the newly allocated context is the first one. Since
////		rt->state is DOWN, it releases the GC lock and starts the first context
////		initialization procedure.
////		RUN_UNTIL(BY(t3), ENTERS(f_beginrequest), "Run TC until js_BeginRequest");
//
//		MAX_WAIT_TIME(3*USECSPERSEC);
//
////		That procedure includes the allocation of the initial
////		atoms and it will happen when the thread A runs the GC. This may lead precisely
////		to the first stack trace from the comment 4.
//		WHILE_STAR {
//			FORALL(t, (TID == t1) || (TID == t3));
//			RUN_UNTIL(TID == t, PTRUE, "Run until true...");
////			RUN_UNTIL(TID == t, CALLS() || ENDS(), "Run until calls..."); // 5 schedules
////			RUN_UNTIL(TID == t, ACCESSES() || ENDS(), "Run until accesses...");
////			RUN_UNTIL(TID == t, CALLS() || ACCESSES() || ENDS(), "Run until accesses...");
////			1 schedule. BUG!
////			RUN_UNTIL(TID == t, ACCESSES(RT_STATE) || ACCESSES(RT_GCLOCK) || CALLS() || ENDS(), "Run until accesses...");
//		}
//	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
