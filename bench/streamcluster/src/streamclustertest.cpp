#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {

		MAX_WAIT_TIME(0);

		FUNC(ls, localSearch);
		FUNC(lss, localSearchSub);
		FUNC(median, pkmedian);
		FUNC(barrier, my_pthread_barrier_wait);

		WAIT_FOR_THREAD(tm, IN_FUNC(ls), "Main");
		RUN_UNTIL(BY(tm), HITS_PC(1), "Main creates threads");

		//---------------------------------

		TVAR(t1);
		TVAR(t2);
		TVAR(t3);

		RUN_UNTIL(NOT(tm), HITS_PC(42), t1, "until t1 enters");
		RUN_UNTIL(NOT(tm) && NOT(t1), HITS_PC(42), t2, "until t2 enters");
		RUN_UNTIL(NOT(tm) && NOT(t1) && NOT(t2), HITS_PC(42), t3, "until t3 enters");

		//---------------------------------

#define ENTERS_BARRIER(t)	 (ENTERS(barrier, t) || RETURNS(median, t))
#define EXITS_BARRIER(t)	 (RETURNS(barrier, t) || RETURNS(median, t))

		MAX_WAIT_TIME(3*USECSPERSEC);

		// bind t1 to the first thread that enters the barrier, then 2, and then t3
		RUN_UNTIL(NOT(tm), ENTERS_BARRIER(TID), t1, "until t1 enters");
		RUN_UNTIL(NOT(tm) && NOT(t1), ENTERS_BARRIER(TID), t2, "until t2 enters");
		RUN_UNTIL(NOT(tm) && NOT(t1) && NOT(t2), ENTERS_BARRIER(TID), t3, "until t3 enters");

		WHILE_STAR // each phase
		{
			// bind t1 to the first thread that exits the barrier, then 2, and then t3
			RUN_UNTIL(NOT(tm), EXITS_BARRIER(TID), t1, "until t1 enters");
			RUN_UNTIL(NOT(tm) && NOT(t1), EXITS_BARRIER(TID), t2, "until t2 enters");
			RUN_UNTIL(NOT(tm) && NOT(t1) && NOT(t2), EXITS_BARRIER(TID), t3, "until t3 enters");

			SELECT_THREAD_BACKTRACK(tt1, TID == t1 || TID == t2 || TID == t3, "Select tt1");
			FORALL(tt2, (TID == t1 || TID == t2 || TID == t3) && NOT(tt1), "Select tt1");
			FORALL(tt3, (TID == t1 || TID == t2 || TID == t3) && NOT(tt1) && NOT(tt2), "Select tt1");

			RUN_UNTIL(BY(tt1), ENTERS_BARRIER(tt1), "Run tt1 until ...");
			RUN_UNTIL(BY(tt2), ENTERS_BARRIER(tt2), "Run tt2 until ...");
			RUN_UNTIL(BY(tt3), ENTERS_BARRIER(tt3), "Run tt3 until ...");
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
