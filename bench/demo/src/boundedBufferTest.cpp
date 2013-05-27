#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(BBScenario, "Bounded buffer scenario")

	TESTCASE() {
		MAX_WAIT_TIME(0);

		CALL_TEST(SearchAll);
	}

	//============================================================//

	TEST(SearchSequential) {

		FUNCT(producer_routine);
		FUNCT(consumer_routine);

		TVAR(P1);
		TVAR(P2);
		TVAR(C1);
		TVAR(C2);

		WAIT_FOR_DISTINCT_THREADS((P1, P2), ENTERS(producer_routine), "Wait for 2 producers.");

		WAIT_FOR_DISTINCT_THREADS((C1, C2), ENTERS(consumer_routine), "Wait for 2 consumers.");

		MAX_WAIT_TIME(USECSPERSEC);

		WHILE_STAR {

			TVAR(t);

			CHOOSE_THREAD_BACKTRACK(t, (P1, P2, C1, C2), PTRUE, "Select a thread to execute.");

			// std::cout << "Selected thread id is " << t->ToString() << std::endl;

			RUN_THREAD_THROUGH(t, ENDS(), "Run t until ends.");
		}
	}


	//============================================================//

	TEST(SearchAll) {

		FUNCT(producer_routine);
		FUNCT(consumer_routine);

		TVAR(P1);
		TVAR(P2);
		TVAR(C1);
		TVAR(C2);

		WAIT_FOR_DISTINCT_THREADS((P1, P2), ENTERS(producer_routine), "Wait for 2 producers.");

		WAIT_FOR_DISTINCT_THREADS((C1, C2), ENTERS(consumer_routine), "Wait for 2 consumers.");

		MAX_WAIT_TIME(USECSPERSEC);

		WHILE_STAR {

			TVAR(t);

			CHOOSE_THREAD_BACKTRACK(t, (P1, P2, C1, C2), PTRUE, "Select a thread to execute.");

			RUN_THREAD_THROUGH(t, READS() || WRITES() || CALLS() || HITS_PC() || ENDS(), "Run t until any event.");
		}
	}

	//============================================================//


	TEST(SearchInFunc) {

		FUNCT(bounded_buf_get);
		FUNCT(bounded_buf_put);

		TVAR(P1);
		TVAR(P2);
		TVAR(C1);
		TVAR(C2);

		WAIT_FOR_DISTINCT_THREADS((P1, P2), ENTERS(bounded_buf_put), "Wait for 2 producers.");

		WAIT_FOR_DISTINCT_THREADS((C1, C2), ENTERS(bounded_buf_get), "Wait for 2 consumers.");

		MAX_WAIT_TIME(3*USECSPERSEC);

		WHILE_STAR {

			TVAR(t);

			CHOOSE_THREAD_BACKTRACK(t, (P1, P2, C1, C2), PTRUE, "Select a thread to execute.");

			RUN_THREAD_THROUGH(t, READS() || WRITES() || CALLS() || HITS_PC() || ENDS(), "Run t until any event.");
		}
	}

	//============================================================//

	TEST(SearchInFunc2) {

		FUNCT(bounded_buf_get);
		FUNCT(bounded_buf_put);

		TVAR(P1);
		TVAR(C1);

		WAIT_FOR_THREAD(P1, ENTERS(bounded_buf_put), "Wait for a producers.");

		WAIT_FOR_THREAD(C1, ENTERS(bounded_buf_get), "Wait for a consumers.");

		MAX_WAIT_TIME(3*USECSPERSEC);

		WHILE_STAR {

			TVAR(t);

			CHOOSE_THREAD_BACKTRACK(t, (P1, C1), PTRUE, "Select a thread to execute.");

			RUN_THREAD_THROUGH(t, READS() || WRITES() || CALLS() || HITS_PC() || ENDS(), "Run t until any event.");
		}
	}

	//============================================================//

	TEST(SearchInFuncWith2Thread) {

		FUNCT(bounded_buf_get);
		FUNCT(bounded_buf_put);

		TVAR(P1);
		TVAR(C1);

		WAIT_FOR_THREAD(P1, ENTERS(bounded_buf_put), "Wait for a producers.");

		WAIT_FOR_THREAD(C1, ENTERS(bounded_buf_get), "Wait for a consumers.");

		MAX_WAIT_TIME(3*USECSPERSEC);

		WHILE_STAR {

			TVAR(t);

			CHOOSE_THREAD_BACKTRACK(t, (P1, C1), PTRUE, "Select a thread to execute.");

			RUN_THREAD_THROUGH(t, READS() || WRITES() || CALLS() || HITS_PC() || ENDS(), "Run t until any event.");
		}
	}

CONCURRIT_END_TEST(BBScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
