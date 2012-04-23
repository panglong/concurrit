#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {

		MAX_WAIT_TIME(10*USECSPERSEC);

		FUNC(fc, consumer);
		FUNC(fd, queueDelete);
		FUNC(fw, fileWriter);

		EXISTS(t_witness, IN_FUNC(fc), "Witness");
		RUN_UNTIL(STEP(t_witness), AT_PC(42), __, "Running witness");

		MAX_WAIT_TIME(5*USECSPERSEC);

		EXISTS(t_writer, IN_FUNC(fw), "Writer");
		RUN_UNTIL(NOT(t_witness) && !IN_FUNC(fd), ENDS(t_writer), __, "Ending writer");

		EXISTS(t_deleter, IN_FUNC(fd), "Deleter");
		RUN_UNTIL(STEP(t_deleter), RETURNS(fd), __, "Deletes");

		RUN_UNTIL(STEP(t_witness), RETURNS(fc), __, "Ending first consumer");
	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
