/**
 * modeltypes.h - Typedefs for modelchecking 
 * 
 * Author - Nick Jalbert (jalbert@eecs.berkeley.edu) 
 *
 * <Legal matter>
 */

#ifndef _HYBRIDTYPES_H_
#define _HYBRIDTYPES_H_

#define MAXTHREADS 1000
#define MAXLOCKS 1000
#define DEBUG true 

#include <map>
#include <set>
#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <pthread.h>
#include <string>
#include <semaphore.h>
#include <errno.h>


using namespace std;

void safe_asrt(bool cond, int ex) {
    if (!cond) {
        cout << "ERROR: safe_assert value " << ex << endl << flush;
        exit(ex);
    }
}

struct Boolwrapper {
    volatile bool var;
};

#endif
