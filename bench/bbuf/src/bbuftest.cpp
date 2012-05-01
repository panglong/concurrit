#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(BBScenario, "Bounded buffer scenario")

	TESTCASE() {

		FVAR(f_get, bounded_buf_get);
		FVAR(f_put, bounded_buf_put);

		EXISTS(t1, IN_FUNC(f_get), "Select t1");
		EXISTS(t2, IN_FUNC(f_get), "Select t2");

		WHILE_STAR
		{
			FORALL(t, BY(t1) || BY(t2), "Select t");

			RUN_UNTIL(BY(t), READS() || WRITES(), __, "Step t");
		}
	}

CONCURRIT_END_TEST(BBScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
