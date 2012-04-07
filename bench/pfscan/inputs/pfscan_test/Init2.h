

#ifndef _INITIALIZER_H_INCLUDED
#define _INITIALIZER_H_INCLUDED

#include "mypthread.h"

class Initializer
{
public:

  static bool is_done();

  static void run();

 private:

  static void _run();

  static pthread_once_t _once_control;

  static volatile bool _done;

};

#endif // #ifndef _INITIALIZER_H_INCLUDED

