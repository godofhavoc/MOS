#include <stdlib.h>
#include <time.h>
#include "local.h"

#define _SEC_IN_MINUTE 60L
#define _SEC_IN_HOUR 3600L
#define _SEC_IN_DAY 86400L

static _CONST int DAYS_IN_MONTH[12] =
{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

#define _DAYS_IN_MONTH(x) ((x == 1) ? days_in_feb : DAYS_IN_MONTH[x])

static _CONST int _DAYS_BEFORE_MONTH[12] =
{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

#define _ISLEAP(y) (((y) % 4) == 0 && (((y) % 100) != 0 || (((y)+1900) % 400) == 0))
#define _DAYS_IN_YEAR(year) (_ISLEAP(year) ? 366 : 365)

static void 
_DEFUN(validate_structure, (tim_p),
     struct tm *tim_p)
{
  div_t res;
  int days_in_feb = 28;

  if (tim_p->tm_sec < 0 || tim_p->tm_sec > 59)
    {
      res = div (tim_p->tm_sec, 60);
      tim_p->tm_min += res.quot;
      if ((tim_p->tm_sec = res.rem) < 0)
	{
	  tim_p->tm_sec += 60;
	  --tim_p->tm_min;
	}
    }

  if (tim_p->tm_min < 0 || tim_p->tm_min > 59)
    {
      res = div (tim_p->tm_min, 60);
      tim_p->tm_hour += res.quot;
      if ((tim_p->tm_min = res.rem) < 0)
	{
	  tim_p->tm_min += 60;
	  --tim_p->tm_hour;
        }
    }

  if (tim_p->tm_hour < 0 || tim_p->tm_hour > 23)
    {
      res = div (tim_p->tm_hour, 24);
      tim_p->tm_mday += res.quot;
      if ((tim_p->tm_hour = res.rem) < 0)
	{
	  tim_p->tm_hour += 24;
	  --tim_p->tm_mday;
        }
    }

  if (tim_p->tm_mon > 11)
    {
      res = div (tim_p->tm_mon, 12);
      tim_p->tm_year += res.quot;
      if ((tim_p->tm_mon = res.rem) < 0)
        {
	  tim_p->tm_mon += 12;
	  --tim_p->tm_year;
        }
    }

  if (_DAYS_IN_YEAR (tim_p->tm_year) == 366)
    days_in_feb = 29;

  if (tim_p->tm_mday <= 0)
    {
      while (tim_p->tm_mday <= 0)
	{
	  if (--tim_p->tm_mon == -1)
	    {
	      tim_p->tm_year--;
	      tim_p->tm_mon = 11;
	      days_in_feb =
		((_DAYS_IN_YEAR (tim_p->tm_year) == 366) ?
		 29 : 28);
	    }
	  tim_p->tm_mday += _DAYS_IN_MONTH (tim_p->tm_mon);
	}
    }
  else
    {
      while (tim_p->tm_mday > _DAYS_IN_MONTH (tim_p->tm_mon))
	{
	  tim_p->tm_mday -= _DAYS_IN_MONTH (tim_p->tm_mon);
	  if (++tim_p->tm_mon == 12)
	    {
	      tim_p->tm_year++;
	      tim_p->tm_mon = 0;
	      days_in_feb =
		((_DAYS_IN_YEAR (tim_p->tm_year) == 366) ?
		 29 : 28);
	    }
	}
    }
}

time_t 
_DEFUN(mktime, (tim_p),
     struct tm *tim_p)
{
  time_t tim = 0;
  long days = 0;
  int year, isdst, tm_isdst;
  __tzinfo_type *tz = __gettzinfo ();

  validate_structure (tim_p);

  tim += tim_p->tm_sec + (tim_p->tm_min * _SEC_IN_MINUTE) +
    (tim_p->tm_hour * _SEC_IN_HOUR);

  days += tim_p->tm_mday - 1;
  days += _DAYS_BEFORE_MONTH[tim_p->tm_mon];
  if (tim_p->tm_mon > 1 && _DAYS_IN_YEAR (tim_p->tm_year) == 366)
    days++;

  tim_p->tm_yday = days;

  if (tim_p->tm_year > 10000
      || tim_p->tm_year < -10000)
    {
      return (time_t) -1;
    }
  if (tim_p->tm_year > 70)
    {
      for (year = 70; year < tim_p->tm_year; year++)
	days += _DAYS_IN_YEAR (year);
    }
  else if (tim_p->tm_year < 70)
    {
      for (year = 69; year > tim_p->tm_year; year--)
	days -= _DAYS_IN_YEAR (year);
      days -= _DAYS_IN_YEAR (year);
    }

  if ((tim_p->tm_wday = (days + 4) % 7) < 0)
    tim_p->tm_wday += 7;

  tim += (days * _SEC_IN_DAY);

  tm_isdst = tim_p->tm_isdst > 0  ?  1 : tim_p->tm_isdst;
  isdst = tm_isdst;

  if (_daylight)
    {
      int y = tim_p->tm_year + YEAR_BASE;
      if (y == tz->__tzyear || __tzcalc_limits (y))
	{
          time_t startdst_dst = tz->__tzrule[0].change
	    - (time_t) tz->__tzrule[1].offset;
	  time_t startstd_dst = tz->__tzrule[1].change
	    - (time_t) tz->__tzrule[1].offset;
	  time_t startstd_std = tz->__tzrule[1].change
	    - (time_t) tz->__tzrule[0].offset;

	  if (tim >= startstd_std && tim < startstd_dst)
	    ; 
          else
	    {
	      isdst = (tz->__tznorth
		       ? (tim >= startdst_dst && tim < startstd_std)
		       : (tim >= startdst_dst || tim < startstd_std));

 	      if (tm_isdst >= 0  &&  (isdst ^ tm_isdst) == 1)
		{
		  int diff = (int) (tz->__tzrule[0].offset
				    - tz->__tzrule[1].offset);
		  if (!isdst)
		    diff = -diff;
		  tim_p->tm_sec += diff;
		  validate_structure (tim_p);
		  tim += diff;  
		}
	    }
	}
    }

  if (isdst == 1)
    tim += (time_t) tz->__tzrule[1].offset;
  else 
    tim += (time_t) tz->__tzrule[0].offset;

  tim_p->tm_isdst = isdst;

  return tim;
}
