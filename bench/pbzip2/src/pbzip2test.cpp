#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {
		CALL_TEST(Final2);
	}

	//============================================================//

	// GLOG_v=0 scripts/run_bench.sh pbzip2 -s -c -m0
	// finfile: queueDelete consumer fileWriter
//	TEST(Modelcheck) {
//
//		MAX_WAIT_TIME(10*USECSPERSEC);
//
//		UNTIL_ALL_END {
//			FORALL(t, PTRUE, "Forall thread");
//			RUN_THREAD_ONCE(t, "Run t once");
//		}
//	}

	//============================================================//

	// GLOG_v=0 scripts/run_bench.sh pbzip2 -s -c -p0
	// finfile: queueDelete
	TEST(Final1) {

		MAX_WAIT_TIME(10*USECSPERSEC);

		FUNC(fc, consumer);
		FUNC(fd, queueDelete);
		FUNC(fw, fileWriter);

		TVAR(t_witness);
		TVAR(t_writer);
		TVAR(t_deleter);

		WAIT_FOR_THREAD(t_witness, ENTERS(fc), "Witness");
		RUN_THREAD_UNTIL(t_witness, HITS_PC(42), "Running witness");

		MAX_WAIT_TIME(5*USECSPERSEC);

		WAIT_FOR_THREAD(t_writer, ENTERS(fw), "Writer");
		RUN_UNTIL(NOT_BY(t_witness) && !IN_FUNC(fd), ENDS(t_writer), "Ending writer");

		WAIT_FOR_THREAD(t_deleter, ENTERS(fd), "Deleter");
		RUN_THREAD_UNTIL(t_deleter, RETURNS(fd), "Deletes");

		RUN_THREAD_UNTIL(t_witness, RETURNS(fc), "Ending first consumer");
	}

	// GLOG_v=0 scripts/run_bench.sh pbzip2 -s -c -p0
	// finfile: queueDelete
	TEST(Final2) {

		MAX_WAIT_TIME(10*USECSPERSEC);

		FUNC(fc, consumer);
		FUNC(fd, queueDelete);
		FUNC(fw, fileWriter);

		TVAR(t_witness);
		TVAR(t_writer);

		WAIT_FOR_THREAD(t_witness, IN_FUNC(fc), "Witness");
		RUN_THREAD_UNTIL(t_witness, HITS_PC(42), "Running witness");

		MAX_WAIT_TIME(5*USECSPERSEC);

		RUN_UNTIL(NOT_BY(t_witness), RETURNS(fd), "Ending deleter");

		RUN_THREAD_UNTIL(t_witness, RETURNS(fc), "Ending first consumer");
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
