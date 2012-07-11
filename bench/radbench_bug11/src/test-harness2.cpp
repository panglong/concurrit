#include <stdio.h>
#include <stdlib.h>

#include "interface.h"
#include "ipc.h"

int main(int argc, char* argv[])
{
	fprintf(stderr, "HARNESS: Openning pipe\n");

	concurrit::ConcurrentPipe* controlpipe = concurrit::ConcurrentPipe::OpenControlForSUT(NULL, false);

	fprintf(stderr, "HARNESS: Starting\n");

	//====================================================
	//====================================================

	for(;;) {

		fprintf(stderr, "HARNESS: Sending TestStart event\n");

		concurrit::EventBuffer e;
		e.type = concurrit::TestStart;
		e.threadid = 0;
		controlpipe->Send(NULL, &e);

		controlpipe->Recv(NULL, &e);
		safe_assert(e.type == Continue);

		fprintf(stderr, "HARNESS: Sent TestStart event\n");

		//====================================================
		//====================================================

		{
			system("/home/elmas/radbench/Benchmarks/bug11/bin/install/bin/ab -n 10 -c 10 -t 30 http://localhost:8091/");
		}

		fprintf(stderr, "HARNESS: Joined threads\n");

		//====================================================
		//====================================================

		e.type = concurrit::TestEnd;
		e.threadid = 0;
		controlpipe->Send(NULL, &e);

		controlpipe->Recv(NULL, &e);
		safe_assert(e.type == Continue);

		fprintf(stderr, "HARNESS: Sent TestEnd event\n");

	} // end for

	controlpipe->Close();

	delete(controlpipe);

	fprintf(stderr, "HARNESS: Exiting\n");

	return 0;
}
