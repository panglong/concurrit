#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {

		Config::MaxWaitTimeUSecs = (5*USECSPERSEC);

//
//		FUNC(fc, consumer);
//		FUNC(fd, queueDelete);
//		FUNC(fw, fileWriter);
//
//		EXISTS(t_writer, IN_FUNC(fw), "Writer");
//
//		Config::ChooseStarRandomly = false;
//		WHILE_STAR {
//			Config::ChooseStarRandomly = true;
//			IF_STAR {
//				EXISTS(t_consumer, (t_writer != t_consumer) && IN_FUNC(fc), "Consumer");
//				RUN_ONCE(!RETURNS(fc), t_consumer, "Running consumer");
//			} ELSE {
//				RUN_ONCE(!RETURNS(fw), t_writer, "Running writer");
//			}
//			Config::ChooseStarRandomly = false;
//		}
//
//		RUN_UNTIL(t_writer, RETURNS(fw), "Ending writer");
//
//		EXISTS(t_deleter, IN_FUNC(fd), "Deleter");
//
//		RUN_UNTIL(t_deleter, RETURNS(fd), "Deletes");


		FUNC(fc, consumer);
		FUNC(fd, queueDelete);
		FUNC(fw, fileWriter);

		EXISTS(tc, IN_FUNC(fc), "Consumer");
		RUN_UNTIL(tc, AT_PC(42), "Running consumer");

		EXISTS(t_consumer, (tc != t_consumer) && IN_FUNC(fc), "Consumer");
		RUN_UNTIL(t_consumer, RETURNS(fc), "Running consumer");

		EXISTS(t_writer, IN_FUNC(fw), "Writer");
		RUN_UNTIL(t_writer, RETURNS(fw), "Ending writer");

		EXISTS(t_deleter, IN_FUNC(fd), "Deleter");

		RUN_UNTIL(t_deleter, RETURNS(fd), "Deletes");

		RUN_UNTIL(tc, RETURNS(fc), "Ending first consumer");


	}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()
