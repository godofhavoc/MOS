
#ifdef CLOCK_PROVIDED

int _dummy_clock = 1;

#else

#include <time.h>
#include <sys/times.h>
#include <reent.h>

clock_t 
clock ()
{
  struct tms tim_s;
  clock_t res;

  if ((res = (clock_t) _times_r (_REENT, &tim_s)) != -1)
    res = (clock_t) (tim_s.tms_utime + tim_s.tms_stime +
		     tim_s.tms_cutime + tim_s.tms_cstime);

  return res;
}

#endif /* CLOCK_PROVIDED */
