#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {
		CALL_TEST(Modelcheck);
	}

	//============================================================//

	// GLOG_v=0 scripts/run_bench.sh pbzip2 -s -c -m0
	// finfile: queueDelete consumer fileWriter
	TEST(Modelcheck) {

		MAX_WAIT_TIME(10*USECSPERSEC);

		UNTIL_ALL_END {
			FORALL(t, PTRUE, "Forall thread");
			RUN_THREAD_ONCE(t, "Run t once");
		}
	}

	//============================================================//

	// GLOG_v=0 scripts/run_bench.sh pbzip2 -s -c -p0
	// finfile: queueDelete
	TEST(Final) {

		MAX_WAIT_TIME(10*USECSPERSEC);

		FUNC(fc, consumer);
		FUNC(fd, queueDelete);
		FUNC(fw, fileWriter);

		WAIT_FOR_THREAD(t_witness, IN_FUNC(fc), "Witness");
		RUN_THREAD_UNLESS(t_witness, HITS_PC(42), "Running witness");

		MAX_WAIT_TIME(5*USECSPERSEC);

		WAIT_FOR_THREAD(t_writer, IN_FUNC(fw), "Writer");
		RUN_UNTIL(NOT_BY(t_witness) && !IN_FUNC(fd), ENDS(t_writer), "Ending writer");

		WAIT_FOR_THREAD(t_deleter, IN_FUNC(fd), "Deleter");
		RUN_THREAD_UNTIL(t_deleter, RETURNS(fd), "Deletes");

		RUN_THREAD_UNTIL(t_witness, RETURNS(fc), "Ending first consumer");
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
