#include <_ansi.h>
#include <reent.h>
#include <sys/types.h>
#include <sys/time.h>

time_t
_DEFUN (time, (t),
	time_t * t)
{
  struct timeval now;

  if (_gettimeofday_r (_REENT, &now, NULL) >= 0)
    {
      if (t)
	*t = now.tv_sec;
      return now.tv_sec;
    }
  return -1;
}
