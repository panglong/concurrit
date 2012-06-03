#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {
		CALL_TEST(ModelCheckGet);
	}

	//================================//

	// GLOG_v=0 scripts/run_bench.sh pfscan -s -c
	// finfile: pqueue_get
	TEST(Final) {
		MAX_WAIT_TIME(3*USECSPERSEC);

		FUNC(fg, pqueue_get);
		FUNC(fp, pqueue_put);

		WAIT_FOR_THREAD(t1, IN_FUNC(fg), "T1");
		WAIT_FOR_DISTINCT_THREAD(t2, IN_FUNC(fg), "T2");

		WHILE(!HAS_ENDED(t1) || !HAS_ENDED(t2)) { // loop to consider other get calls of same threads

			RUN_THREAD_UNLESS(t1, HITS_PC(42), __, "pqueue_get starts by T1");

			RUN_THREAD_UNTIL(t2, RETURNS(fg), __, "pqueue_get by T2");

			RUN_THREAD_UNTIL(t1, RETURNS(fg), __, "pqueue_get ends by T1");
		}
	}

	TEST(ModelCheckGet) {
		MAX_WAIT_TIME(3*USECSPERSEC);

		FUNC(fg, pqueue_get);
		FUNC(fp, pqueue_put);

		EXISTS(t1, IN_FUNC(fg), "T1");
		EXISTS(t2, NOT(t1) && IN_FUNC(fg), "T2");

		WHILE(!HAS_ENDED(t1) || !HAS_ENDED(t2)) {

			SELECT_THREAD_BACKTRACK(t, PTRUE);

			RUN_THREAD_ONCE(t, "Run t until reads or writes memory");
		}
	}

	TEST(ModelCheckAll) {
		MAX_WAIT_TIME(3*USECSPERSEC);

		NDConcurrentSearch(PTRUE, READS() || WRITES() || ENDS());
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
