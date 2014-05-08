#include <time.h>
#include <reent.h>

#ifndef _REENT_ONLY

struct tm *
_DEFUN (localtime, (tim_p),
	_CONST time_t * tim_p)
{
  _REENT_CHECK_TM(_REENT);
  return localtime_r (tim_p, (struct tm *)_REENT_TM(_REENT));
}

#endif
