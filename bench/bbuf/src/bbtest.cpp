#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(BBScenario, "Bounded buffer scenario")

	TESTCASE() {

		WHILE_STAR
		{
//			EXISTS(t);
//			DSLTransition(TransitionPredicate::True(), t);

//			DSLTransition(TransitionPredicate::False());
//			DSLTransition(TransitionPredicate::True());

			EXISTS(t, PTRUE, "t");
//			FORALL(t);
//			TVAR(t);
			RUN_UNTIL(STEP(t), ENDS(), __, "step...");
//			DSLTransition(TransitionPredicate::False(), t);
//			RUN_UNTIL(t, TransitionPredicate::False());
		}
	}

CONCURRIT_END_TEST(BBScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
