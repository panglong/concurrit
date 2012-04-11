#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {

		Config::MaxWaitTimeUSecs = USECSPERSEC;


		FUNC(fc, consumer);
		FUNC(fd, queueDelete);
		FUNC(fw, fileWriter);

		EXISTS(t_consumer, IN_FUNC(fc), "Consumer");
		EXISTS(t_writer, (t_writer != t_consumer) && IN_FUNC(fw), "Writer");

		WHILE_STAR {
			IF_STAR {
				RUN_ONCE(PTRUE, t_consumer, "Running consumer");
			} ELSE {
				RUN_ONCE(PTRUE, t_writer, "Running writer");
			}
		}

		RUN_ONCE(RETURNS(fw), t_writer, "Running writer");

		EXISTS(t_deleter, (t_writer != t_consumer) && (t_writer != t_deleter) && IN_FUNC(fd), "Deleter");

		RUN_UNTIL(t_deleter, RETURNS(fd), "Deletes");

	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
