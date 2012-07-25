#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {
		CALL_TEST(SearchInLargeSteps2);
	}

	// uses only manual instr.
	// GLOG_v=0 scripts/run_bench.sh streamcluster -c -s -r -p0
	TEST(SearchInLargeSteps) {

		MAX_WAIT_TIME(0);

		FUNC(ls, localSearch);
		FUNC(lss, localSearchSub);
		FUNC(median, pkmedian);
		FUNC(barrier, my_pthread_barrier_wait);


		TVAR(tm);
		WAIT_FOR_THREAD(tm, ENTERS(ls), "Main");
		RUN_UNTIL(BY(tm), HITS_PC(1), "Main creates threads");

		//---------------------------------

		TVAR(t1);
		TVAR(t2);
		TVAR(t3);

		RUN_UNTIL(ANY_THREAD - tm, HITS_PC(42), t1, "t1");
		RUN_UNTIL(ANY_THREAD - tm - t1, HITS_PC(42), t2, "t2");
		RUN_UNTIL(ANY_THREAD - tm - t1 - t2, HITS_PC(42), t3, "t3");

//		RUN_THREAD_UNTIL_NEXT(tm, "tm until next");


#define ENTERS_BARRIER(t)	 (ENTERS(barrier, t) || ENDS(t))
#define EXITS_BARRIER(t)	 (RETURNS(barrier, t) || ENDS(t))

		MAX_WAIT_TIME(5*USECSPERSEC);

		RUN_THREAD_UNTIL(t1, ENTERS_BARRIER(TID), "until t1 enters");
		RUN_THREAD_UNTIL(t2, ENTERS_BARRIER(TID), "until t2 enters");

		WHILE(!ENDS(t1)->EvalState(t1))
		{
			RUN_THREAD_UNTIL(t3, ENTERS_BARRIER(TID), "until t3 enters");

			TVAR(tt1);
			TVAR(tt2);
			TVAR(tt3);

			RELEASE(t1, "t1 until next");
			RELEASE(t2, "t2 until next");
			RELEASE(t3, "t3 until next");

			// the exit barrier
			RUN_UNTIL(ANY_THREAD - tm, EXITS_BARRIER(TID), tt1, "until t1 exit");
			RUN_UNTIL(ANY_THREAD - tm - tt1, EXITS_BARRIER(TID), tt2, "until t2 exit");
			RUN_UNTIL(ANY_THREAD - tm - tt1 - tt2, EXITS_BARRIER(TID), tt3, "until t3 exit");

			if(ENDS(t1)->EvalState(t1)) break;

			RUN_THREAD_UNTIL(t1, ENTERS_BARRIER(TID), "until t1 enters");
			RUN_THREAD_UNTIL(t2, ENTERS_BARRIER(TID), "until t2 enters");
		}


		RUN_THREAD_THROUGH(t1, ENDS(), "t1 ends");
		RUN_THREAD_THROUGH(t2, ENDS(), "t2 ends");

		RUN_THREAD_THROUGH(tm, ENDS(), "main ends");

		RUN_THREAD_THROUGH(t3, ENDS(), "t3 ends");
	}


	//=====================================================================================


	// uses only manual instr.
	// GLOG_v=0 scripts/run_bench.sh streamcluster -c -s -r -p0
	TEST(SearchInLargeSteps2) {

		MAX_WAIT_TIME(0);

		FUNC(ls, localSearch);
		FUNC(lss, localSearchSub);
		FUNC(median, pkmedian);
		FUNC(barrier, my_pthread_barrier_wait);


		TVAR(tm);
		WAIT_FOR_THREAD(tm, IN_FUNC(ls), "Main");
		RUN_UNTIL(BY(tm), HITS_PC(1), "Main creates threads");

		//---------------------------------

		TVAR(t1);
		TVAR(t2);
		TVAR(t3);

		WAIT_FOR_DISTINCT_THREADS((t1, t2, t3), ENTERS(median), "t1, t2, and t3");

		//---------------------------------

#define ENTERS_BARRIER(t)	 (ENTERS(barrier, t) || ENDS(t))
#define EXITS_BARRIER(t)	 (RETURNS(barrier, t) || ENDS(t))

		MAX_WAIT_TIME(3*USECSPERSEC);

		// bind t1 to the first thread that enters the barrier, then 2, and then t3
		RUN_UNTIL(ANY_THREAD - tm, ENTERS_BARRIER(TID), t1, "until t1 enters");
		RUN_UNTIL(ANY_THREAD - tm - t1, ENTERS_BARRIER(TID), t2, "until t2 enters");
		RUN_UNTIL(ANY_THREAD - tm - t1 - t2, ENTERS_BARRIER(TID), t3, "until t3 enters");

		WHILE(IN_FUNC(median, t1)->EvalState() || IN_FUNC(median, t2)->EvalState() || IN_FUNC(median, t3)->EvalState())
		{
			TVAR(tt1);
			TVAR(tt2);
			TVAR(tt3);

			RELEASE(t1, "t1 until next");
			RELEASE(t2, "t1 until next");
			RELEASE(t3, "t1 until next");

			// bind t1 to the first thread that exits the barrier, then 2, and then t3
			RUN_UNTIL(ANY_THREAD - tm, EXITS_BARRIER(TID), t1, "until t1 enters");
			RUN_UNTIL(ANY_THREAD - tm - t1, EXITS_BARRIER(TID), t2, "until t2 enters");
			RUN_UNTIL(ANY_THREAD - tm - t1 - t2, EXITS_BARRIER(TID), t3, "until t3 enters");

			CHOOSE_THREAD_BACKTRACK(tt1, (t1, t2, t3), PTRUE, "Select tt1");
			FORALL(tt2, (TID == t1 || TID == t2 || TID == t3) && NOT(tt1), "Select tt2");
			FORALL(tt3, (TID == t1 || TID == t2 || TID == t3) && NOT(tt1) && NOT(tt2), "Select tt3");

			RELEASE(t1, "t1 until next");
			RELEASE(t2, "t1 until next");
			RELEASE(t3, "t1 until next");

			RUN_UNTIL(BY(tt1), ENTERS_BARRIER(tt1), "Run tt1 until ...");
			RUN_UNTIL(BY(tt2), ENTERS_BARRIER(tt2), "Run tt2 until ...");
			RUN_UNTIL(BY(tt3), ENTERS_BARRIER(tt3), "Run tt3 until ...");
		}


		RUN_THREAD_THROUGH(t1, ENDS(), "ttt1 ends");
		RUN_THREAD_THROUGH(t2, ENDS(), "ttt2 ends");

		RUN_THREAD_UNTIL(tm, ENDS(), "main returns");

		RUN_THREAD_UNTIL(t3, ENDS(), "ttt3 ends");
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
