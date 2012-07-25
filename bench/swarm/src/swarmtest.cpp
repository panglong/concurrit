#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {

		FUNC(f_sort, radixsort_swarm_s3);
		FUNC(f_csort, countsort_swarm);
		FUNC(f_barrier, SWARM_Barrier_sync);

		MAX_WAIT_TIME(0);

		TVAR(t1);
		TVAR(t2);
		WAIT_FOR_DISTINCT_THREADS((t1, t2), IN_FUNC(f_sort), "Select t1 and t2");

		MAX_WAIT_TIME(3*USECSPERSEC);

		WHILE_STAR
		{
			TVAR(t);
			CHOOSE_THREAD_BACKTRACK(t, (t1, t2), "Select t");

			RUN_UNTIL(BY(t), (READS() || WRITES() || ENTERS(f_barrier) || ENDS()), "Run t until ...");
			WHILE_STAR {
				RUN_UNTIL(BY(t), (READS() || WRITES() || ENTERS(f_barrier) || ENDS()), "Run t until ...");
			}
		}
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
