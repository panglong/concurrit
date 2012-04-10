#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(BBScenario, "Bounded buffer scenario")

	TESTCASE() {

		TEST_FORALL();

		WHILE_STAR
		{
//			EXISTS(t);
//			DSLTransition(TransitionPredicate::True(), t);

//			DSLTransition(TransitionPredicate::False());
			DSLTransition(TransitionPredicate::True());

//			FORALL(t);
//			EXISTS(t);
//			DSLTransferUntil(t, ENDS());
//			DSLTransferUntil(t, TransitionPredicate::True());
		}
	}

CONCURRIT_END_TEST(BBScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
