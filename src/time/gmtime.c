#include <stdlib.h>
#include <time.h>

#define _GMT_OFFSET 0

#ifndef _REENT_ONLY

struct tm *
_DEFUN (gmtime, (tim_p),
	_CONST time_t * tim_p)
{
  _REENT_CHECK_TM(_REENT);
  return gmtime_r (tim_p, (struct tm *)_REENT_TM(_REENT));
}

#endif
