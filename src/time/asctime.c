#include <time.h>
#include <_ansi.h>
#include <reent.h>

#ifndef _REENT_ONLY

char *
_DEFUN (asctime, (tim_p),
	_CONST struct tm *tim_p)
{
  _REENT_CHECK_ASCTIME_BUF(_REENT);
  return asctime_r (tim_p, _REENT_ASCTIME_BUF(_REENT));
}

#endif
