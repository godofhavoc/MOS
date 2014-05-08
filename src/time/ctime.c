#include <time.h>

#ifndef _REENT_ONLY

char *
_DEFUN (ctime, (tim_p),
	_CONST time_t * tim_p)
{
  return asctime (localtime (tim_p));
}

#endif
