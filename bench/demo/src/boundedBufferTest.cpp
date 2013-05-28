#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(BBScenario, "Bounded buffer scenario")

	TESTCASE() {
		MAX_WAIT_TIME(0);

		FUNCT(producer_routine);
		FUNCT(consumer_routine);

		TVAR(P1);
		TVAR(P2);
		TVAR(C1);
		TVAR(C2);

		WAIT_FOR_DISTINCT_THREADS((P1, P2), ENTERS(producer_routine), "Wait for 2 producers.");

		WAIT_FOR_DISTINCT_THREADS((C1, C2), ENTERS(consumer_routine), "Wait for 2 consumers.");

		MAX_WAIT_TIME(USECSPERSEC);

		WHILE (!ALL_ENDED(P1, P2, C1, C2)) {

			TVAR(t);

			CHOOSE_THREAD_BACKTRACK(t, (P1, P2, C1, C2), PTRUE, "Select a thread to execute.");

			RUN_THREAD_THROUGH(t, READS() || WRITES() || CALLS() || ENDS(), "Run t until any event.");
		}
	}

CONCURRIT_END_TEST(BBScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
