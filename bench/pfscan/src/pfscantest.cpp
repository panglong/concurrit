#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {
		CALL_TEST(ModelCheckGetPut);
	}

	//================================//

	TEST(Final) {
		MAX_WAIT_TIME(3*USECSPERSEC);

		FUNC(fg, pqueue_get);
		FUNC(fp, pqueue_put);

		EXISTS(t1, IN_FUNC(fg), "T1");
		EXISTS(t2, NOT(t1) && IN_FUNC(fg), "T2");

		WHILE_STAR {

			RUN_UNTIL(BY(t1), AT_PC(42), __, "pqueue_get starts by T1");

			RUN_UNTIL(BY(t2), RETURNS(fg), __, "pqueue_get by T2");

			RUN_UNTIL(BY(t1), RETURNS(fg), __, "pqueue_get ends by T1");
		}
	}

	TEST(ModelCheckGetPut) {
		MAX_WAIT_TIME(3*USECSPERSEC);

		FUNC(fg, pqueue_get);
		FUNC(fp, pqueue_put);

		EXISTS(t1, IN_FUNC(fg), "T1");
		EXISTS(t2, NOT(t1) && IN_FUNC(fg), "T2");

		WHILE(!HAS_ENDED(t1) || !HAS_ENDED(t2)) {

			FORALL(t, TID == t1 || TID == t2);

			RUN_UNTIL(BY(t), READS() || WRITES(), __, "Run t until reads or writes memory");
		}
	}

	TEST(ModelCheckAll) {
		MAX_WAIT_TIME(3*USECSPERSEC);

		NDConcurrentSearch(PTRUE, READS() || WRITES());
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
