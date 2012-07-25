#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "MyScenario")

	TESTCASE() {
		CALL_TEST(SearchSeqFunc);
	}

	//====================================================================

	// GLOG_v=0 scripts/run_bench.sh radbench_bug6 -s -c -r -m1 -p1
	TEST(SearchSeqFunc) {

		FVAR(f_reader_main, reader_main);
		FVAR(f_writer_main, writer_main);

		FVAR(f_wlock, PR_RWLock_Wlock);
		FVAR(f_rlock, PR_RWLock_Rlock);
		FVAR(f_unlock, PR_RWLock_Unlock);

		MAX_WAIT_TIME(3*USECSPERSEC);


		TVAR(t1);
		TVAR(t2);

		WAIT_FOR_THREAD(t1, ENTERS(f_reader_main), "reader");
		WAIT_FOR_THREAD(t2, ENTERS(f_writer_main), "writer");

		WHILE_STAR {
			TVAR(t);
			CHOOSE_THREAD_BACKTRACK(t, (t1, t2), PTRUE, "Select t");
			RUN_THREAD_THROUGH(t, RETURNS(f_rlock) || RETURNS(f_wlock) || RETURNS(f_unlock) || ENDS(), "Run t until returns");
		}
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
