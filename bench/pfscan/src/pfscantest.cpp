#include <stdio.h>

#include "concurrit.h"

extern "C" {
#include "pqueue.h"
}

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {

//		CONFIG("-w" + to_string(5 * USECSPERSEC));
		Config::MaxWaitTimeUSecs = USECSPERSEC;

		FUNC(fg, pqueue_get);
		FUNC(fp, pqueue_put);

		EXISTS(t1, IN_FUNC(fg), "T1");
		EXISTS(t2, (t1 != t2) && IN_FUNC(fg), "T2");

		RUN_UNTIL(t1, AT_PC(42), "pqueue_get by T1");

		RUN_UNTIL(t2, RETURNS(fg), "pqueue_get by T2");

	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
