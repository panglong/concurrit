#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {

//	NDConcurrentSearch();

	FUNC(f_pr, PR_LocalTimeParameters);
//	FUNC(f_mt, MT_safe_localtime);

	MAX_WAIT_TIME(0);

	EXISTS(t1, IN_FUNC(f_pr), "Select t1");
	EXISTS(t2, NOT(t1) && IN_FUNC(f_pr), "Select t2");

	MAX_WAIT_TIME(USECSPERSEC);

//	WHILE_STAR {
//		FORALL(t, BY(t1) || BY(t2));
//		RUN_UNTIL(BY(t), READS_WRITES_OR_ENDS(ANY_ADDR, t), __, "Run t until ...");
//	}

	NDConcurrentSearch(BY(t1) || BY(t2));
}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
