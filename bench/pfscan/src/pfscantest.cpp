#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {

		MAX_WAIT_TIME(3*USECSPERSEC);

		FUNC(fg, pqueue_get);
		FUNC(fp, pqueue_put);

		EXISTS(t1, IN_FUNC(fg), "T1");
		EXISTS(t2, NOT(t1) && IN_FUNC(fg), "T2");

//		WHILE_STAR {

			RUN_UNTIL(STEP(t1), ENTERS(fg), __, "pqueue_get by T1 (1)");

			RUN_UNTIL(STEP(t1), AT_PC(42), __, "pqueue_get by T1");

	//		RUN_UNTIL(NOT(t1), RETURNS2(fg, t2), __, "pqueue_get by T2");
			RUN_UNTIL(STEP(t2), RETURNS(fg), __, "pqueue_get by T2");

			RUN_UNTIL(STEP(t1), RETURNS(fg), __, "pqueue_get by T1 (2)");
//		}


			RUN_UNTIL(STEP(t1), ENTERS(fg), __, "pqueue_get by T1 (1)");
			RUN_UNTIL(STEP(t1), AT_PC(42), __, "pqueue_get by T1");

				//		RUN_UNTIL(NOT(t1), RETURNS2(fg, t2), __, "pqueue_get by T2");
						RUN_UNTIL(STEP(t2), RETURNS(fg), __, "pqueue_get by T2");

						RUN_UNTIL(STEP(t1), RETURNS(fg), __, "pqueue_get by T1 (2)");

						RUN_UNTIL(STEP(t1), AT_PC(42), __, "pqueue_get by T1");

							//		RUN_UNTIL(NOT(t1), RETURNS2(fg, t2), __, "pqueue_get by T2");
									RUN_UNTIL(STEP(t2), RETURNS(fg), __, "pqueue_get by T2");

									RUN_UNTIL(STEP(t1), RETURNS(fg), __, "pqueue_get by T1 (2)");

									RUN_UNTIL(STEP(t1), ENTERS(fg), __, "pqueue_get by T1 (1)");
									RUN_UNTIL(STEP(t1), AT_PC(42), __, "pqueue_get by T1");

										//		RUN_UNTIL(NOT(t1), RETURNS2(fg, t2), __, "pqueue_get by T2");
												RUN_UNTIL(STEP(t2), RETURNS(fg), __, "pqueue_get by T2");

												RUN_UNTIL(STEP(t1), RETURNS(fg), __, "pqueue_get by T1 (2)");
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
