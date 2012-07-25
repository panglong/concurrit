#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {
		CALL_TEST(ExactSchedule);
	}

	//============================================================//

	// GLOG_v=0 scripts/run_bench.sh pbzip2 -s -c -m0
	// finfile: queueDelete consumer fileWriter
	TEST(SearchAll) {

		MAX_WAIT_TIME(5*USECSPERSEC);

		WHILE_STAR {
			TVAR(t);
			CHOOSE_THREAD_BACKTRACK(t, (), PTRUE, "Select");
			RELEASE(t, "Next");
			RUN_THREAD_UNTIL(t, READS() || WRITES() || CALLS() || HITS_PC() || ENDS(), "Run t");
		}
	}

	//============================================================//

	// GLOG_v=0 scripts/run_bench.sh pbzip2 -s -c -p0
	// finfile: queueDelete consumer fileWriter
	TEST(SearchInLargeSteps) {

		MAX_WAIT_TIME(3*USECSPERSEC);

		WHILE_STAR {
			TVAR(t);
			CHOOSE_THREAD_BACKTRACK(t, (), PTRUE, "Select");
			RELEASE(t, "Next");
			RUN_THREAD_UNTIL(t, HITS_PC() || ENTERS() || RETURNS() || ENDS(), "Run t");
		}
	}

	//============================================================//

	// GLOG_v=0 scripts/run_bench.sh pbzip2 -s -c -p0
	// finfile: queueDelete
	TEST(ExactSchedule) {

		MAX_WAIT_TIME(10*USECSPERSEC);

		FUNC(fc, consumer);
		FUNC(fd, queueDelete);
		FUNC(fw, fileWriter);

		TVAR(t_witness);
		TVAR(t_writer);

		WAIT_FOR_THREAD(t_witness, IN_FUNC(fc), "Witness");
		RUN_THREAD_UNTIL(t_witness, HITS_PC(42), "Running witness");

		MAX_WAIT_TIME(5*USECSPERSEC);

		RUN_THREADS_UNTIL(ANY_THREAD - t_witness, RETURNS(fd), "Ending deleter");

		RUN_THREAD_UNTIL(t_witness, RETURNS(fc), "Ending first consumer");
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
