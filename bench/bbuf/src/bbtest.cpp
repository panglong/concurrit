#include <stdio.h>

#include "boundedBuffer.h"

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(BBScenario, "Bounded buffer scenario")

	TESTCASE() {

		TEST_FORALL();

		RunTestDriver();


		while(STAR)
		{
//			EXISTS(t);
//			DSLTransition(TransitionPredicate::True(), t);

//			DSLTransition(TransitionPredicate::False());

			EXISTS(t);
			DSLTransferUntil(t, ENDS());
		}
	}

CONCURRIT_END_TEST(BBScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
