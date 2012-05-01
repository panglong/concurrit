#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(BBScenario, "Bounded buffer scenario")

	TESTCASE() {

		FVAR(f_get, bounded_buf_get);
		FVAR(f_put, bounded_buf_put);

		MAX_WAIT_TIME(0);

		EXISTS(t1, IN_FUNC(f_get), "Select t1");
		EXISTS(t2, IN_FUNC(f_get), "Select t2");

		MAX_WAIT_TIME(USECSPERSEC);

		WHILE_STAR
		{
			FORALL(t, BY(t1) || BY(t2), "Select t");

			RUN_UNTIL(BY(t), READS() || WRITES() || HITS_PC(), "Step t");
		}
	}

CONCURRIT_END_TEST(BBScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
