#include <time.h>

double
_DEFUN (difftime, (tim1, tim2),
	time_t tim1 _AND
	time_t tim2)
{
  return (double)(tim1 - tim2);
}
