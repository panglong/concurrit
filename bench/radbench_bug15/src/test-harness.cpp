#include <stdio.h>
#include <stdlib.h>

#include "interface.h"
#include "ipc.h"

unsigned int g_loop = 10;
unsigned int g_threads = 10;

void* worker1(void*) {

//	for(int i = 0; i < 10; ++i) {
		system("/home/elmas/radbench/Benchmarks/bug15/bin/bin/mysql -D test -e 'flush logs;'");
//	}

	return NULL;
}

void* worker2(void*) {

//	for(int i = 0; i < 10; ++i) {
		system("/home/elmas/radbench/Benchmarks/bug15/bin/bin/mysql -D test -e 'insert into a values (1337);'");
//	}

	return NULL;
}

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
			int i;
			pthread_t threads[g_threads];

			int err;
			err = pthread_create(&threads[0], NULL, worker1, NULL);
			err = pthread_create(&threads[1], NULL, worker1, NULL);
			err = pthread_create(&threads[2], NULL, worker2, NULL);

			void* ret;
			err = pthread_join(threads[0], &ret);
			err = pthread_join(threads[1], &ret);
			err = pthread_join(threads[2], &ret);

			system("/home/elmas/radbench/Benchmarks/bug15/bin/bin/mysql -D test -e 'flush logs;'");

			system("/home/elmas/radbench/Benchmarks/bug15/bin/bin/mysql -D test -e 'truncate a;'");
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
