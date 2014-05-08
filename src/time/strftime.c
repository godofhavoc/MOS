#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "local.h"

#if !defined(MAKE_WCSFTIME)
#  define CHAR		char		
#  define CQ(a)		a		
#  define SFLG				
# else
#  define strftime	wcsftime	
#  define CHAR		wchar_t		
#  define CQ(a)		L##a		
#  define snprintf	swprintf	
#  define strncmp	wcsncmp		
#  define SFLG		"l"		
#endif  

#if YEAR_BASE < 0
#  error "YEAR_BASE < 0"
#endif

static _CONST int dname_len[7] =
{6, 6, 7, 9, 8, 6, 8};

static _CONST CHAR *_CONST dname[7] =
{CQ("Sunday"), CQ("Monday"), CQ("Tuesday"), CQ("Wednesday"),
 CQ("Thursday"), CQ("Friday"), CQ("Saturday")};

static _CONST int mname_len[12] =
{7, 8, 5, 5, 3, 4, 4, 6, 9, 7, 8, 8};

static _CONST CHAR *_CONST mname[12] =
{CQ("January"), CQ("February"), CQ("March"), CQ("April"),
 CQ("May"), CQ("June"), CQ("July"), CQ("August"),
 CQ("September"), CQ("October"), CQ("November"), CQ("December")};

static int
_DEFUN (iso_year_adjust, (tim_p),
	_CONST struct tm *tim_p)
{
  int leap = isleap (tim_p->tm_year + (YEAR_BASE
				       - (tim_p->tm_year < 0 ? 0 : 2000)));

#define PACK(yd, wd, lp) (((yd) << 4) + (wd << 1) + (lp))
  switch (PACK (tim_p->tm_yday, tim_p->tm_wday, leap))
    {
    case PACK (0, 5, 0): 
    case PACK (0, 6, 0): 
    case PACK (0, 0, 0): 
    case PACK (0, 5, 1): 
    case PACK (0, 6, 1): 
    case PACK (0, 0, 1): 
    case PACK (1, 6, 0): 
    case PACK (1, 0, 0): 
    case PACK (1, 6, 1): 
    case PACK (1, 0, 1): 
    case PACK (2, 0, 0): 
    case PACK (2, 0, 1): 
      return -1; 
    case PACK (362, 1, 0): 
    case PACK (363, 1, 1): 
    case PACK (363, 1, 0): 
    case PACK (363, 2, 0): 
    case PACK (364, 1, 1): 
    case PACK (364, 2, 1): 
    case PACK (364, 1, 0): 
    case PACK (364, 2, 0): 
    case PACK (364, 3, 0): 
    case PACK (365, 1, 1): 
    case PACK (365, 2, 1): 
    case PACK (365, 3, 1): 
      return 1; 
    }
  return 0; 
#undef PACK
}

size_t
_DEFUN (strftime, (s, maxsize, format, tim_p),
	CHAR *s _AND
	size_t maxsize _AND
	_CONST CHAR *format _AND
	_CONST struct tm *tim_p)
{
  size_t count = 0;
  int i, len;

  for (;;)
    {
      while (*format && *format != CQ('%'))
	{
	  if (count < maxsize - 1)
	    s[count++] = *format++;
	  else
	    return 0;
	}

      if (*format == CQ('\0'))
	break;

      format++;
      if (*format == CQ('E') || *format == CQ('O'))
	format++;

      switch (*format)
	{
	case CQ('a'):
	  for (i = 0; i < 3; i++)
	    {
	      if (count < maxsize - 1)
		s[count++] =
		  dname[tim_p->tm_wday][i];
	      else
		return 0;
	    }
	  break;
	case CQ('A'):
	  for (i = 0; i < dname_len[tim_p->tm_wday]; i++)
	    {
	      if (count < maxsize - 1)
		s[count++] =
		  dname[tim_p->tm_wday][i];
	      else
		return 0;
	    }
	  break;
	case CQ('b'):
	case CQ('h'):
	  for (i = 0; i < 3; i++)
	    {
	      if (count < maxsize - 1)
		s[count++] =
		  mname[tim_p->tm_mon][i];
	      else
		return 0;
	    }
	  break;
	case CQ('B'):
	  for (i = 0; i < mname_len[tim_p->tm_mon]; i++)
	    {
	      if (count < maxsize - 1)
		s[count++] =
		  mname[tim_p->tm_mon][i];
	      else
		return 0;
	    }
	  break;
	case CQ('c'):
	  {
	    size_t adjust = strftime (&s[count], maxsize - count,
				      CQ("%a %b %e %H:%M:%S %Y"), tim_p);
	    if (adjust > 0)
	      count += adjust;
	    else
	      return 0;
	  }
	  break;
	case CQ('C'):
	  {
	    int neg = tim_p->tm_year < -YEAR_BASE;
	    int century = tim_p->tm_year >= 0
	      ? tim_p->tm_year / 100 + YEAR_BASE / 100
	      : abs (tim_p->tm_year + YEAR_BASE) / 100;
            len = snprintf (&s[count], maxsize - count, CQ("%s%.*d"),
                               neg ? CQ("-") : CQ(""), 2 - neg, century);
            if (len < 0  ||  (count+=len) >= maxsize)  return 0;
	  }
	  break;
	case CQ('d'):
	case CQ('e'):
	  len = snprintf (&s[count], maxsize - count,
			*format == CQ('d') ? CQ("%.2d") : CQ("%2d"),
			tim_p->tm_mday);
	  if (len < 0  ||  (count+=len) >= maxsize)  return 0;
	  break;
	case CQ('D'):
	case CQ('x'):
	  
	  len = snprintf (&s[count], maxsize - count,
			CQ("%.2d/%.2d/%.2d"),
			tim_p->tm_mon + 1, tim_p->tm_mday,
			tim_p->tm_year >= 0 ? tim_p->tm_year % 100
			: abs (tim_p->tm_year + YEAR_BASE) % 100);
          if (len < 0  ||  (count+=len) >= maxsize)  return 0;
	  break;
	case CQ('F'):
	  { 
	    size_t adjust = strftime (&s[count], maxsize - count,
				      CQ("%Y-%m-%d"), tim_p);
	    if (adjust > 0)
	      count += adjust;
	    else
	      return 0;
	  }
          break;
	case CQ('g'):
	  {
	    int adjust = iso_year_adjust (tim_p);
	    int year = tim_p->tm_year >= 0 ? tim_p->tm_year % 100
		: abs (tim_p->tm_year + YEAR_BASE) % 100;
	    if (adjust < 0 && tim_p->tm_year <= -YEAR_BASE)
		adjust = 1;
	    else if (adjust > 0 && tim_p->tm_year < -YEAR_BASE)
		adjust = -1;
	    len = snprintf (&s[count], maxsize - count, CQ("%.2d"),
		       ((year + adjust) % 100 + 100) % 100);
            if (len < 0  ||  (count+=len) >= maxsize)  return 0;
	  }
          break;
	case CQ('G'):
	  {
	    int neg = tim_p->tm_year < -YEAR_BASE;
	    int adjust = iso_year_adjust (tim_p);
	    int century = tim_p->tm_year >= 0
	      ? tim_p->tm_year / 100 + YEAR_BASE / 100
	      : abs (tim_p->tm_year + YEAR_BASE) / 100;
	    int year = tim_p->tm_year >= 0 ? tim_p->tm_year % 100
	      : abs (tim_p->tm_year + YEAR_BASE) % 100;
	    if (adjust < 0 && tim_p->tm_year <= -YEAR_BASE)
	      neg = adjust = 1;
	    else if (adjust > 0 && neg)
	      adjust = -1;
	    year += adjust;
	    if (year == -1)
	      {
		year = 99;
		--century;
	      }
	    else if (year == 100)
	      {
		year = 0;
		++century;
	      }
            len = snprintf (&s[count], maxsize - count, CQ("%s%.*d%.2d"),
                               neg ? CQ("-") : CQ(""), 2 - neg, century, year);
            if (len < 0  ||  (count+=len) >= maxsize)
              return 0;
	  }
          break;
	case CQ('H'):
	case CQ('k'):	
	  len = snprintf (&s[count], maxsize - count,
			*format == CQ('k') ? CQ("%2d") : CQ("%.2d"),
			tim_p->tm_hour);
          if (len < 0  ||  (count+=len) >= maxsize)  return 0;
	  break;
	case CQ('I'):
	case CQ('l'):	
	  {
	    register int  h12;
	    h12 = (tim_p->tm_hour == 0 || tim_p->tm_hour == 12)  ?
						12  :  tim_p->tm_hour % 12;
	    len = snprintf (&s[count], maxsize - count,
			*format == CQ('I') ? CQ("%.2d") : CQ("%2d"),
			h12);
	    if (len < 0  ||  (count+=len) >= maxsize)  return 0;
	  }
	  break;
	case CQ('j'):
	  len = snprintf (&s[count], maxsize - count, CQ("%.3d"),
			tim_p->tm_yday + 1);
          if (len < 0  ||  (count+=len) >= maxsize)  return 0;
	  break;
	case CQ('m'):
	  len = snprintf (&s[count], maxsize - count, CQ("%.2d"),
			tim_p->tm_mon + 1);
          if (len < 0  ||  (count+=len) >= maxsize)  return 0;
	  break;
	case CQ('M'):
	  len = snprintf (&s[count], maxsize - count, CQ("%.2d"),
			tim_p->tm_min);
          if (len < 0  ||  (count+=len) >= maxsize)  return 0;
	  break;
	case CQ('n'):
	  if (count < maxsize - 1)
	    s[count++] = CQ('\n');
	  else
	    return 0;
	  break;
	case CQ('p'):
	  if (count < maxsize - 1)
	    {
	      if (tim_p->tm_hour < 12)
		s[count++] = CQ('A');
	      else
		s[count++] = CQ('P');
	    }
	  if (count < maxsize - 1)
	    {
	      s[count++] = CQ('M');
	    }
	  else
	    return 0;
	  break;
	case CQ('r'):
	  {
	    register int  h12;
	    h12 = (tim_p->tm_hour == 0 || tim_p->tm_hour == 12)  ?
						12  :  tim_p->tm_hour % 12;
	    len = snprintf (&s[count], maxsize - count,
			CQ("%.2d:%.2d:%.2d %cM"),
			h12, 
			tim_p->tm_min,
			tim_p->tm_sec,
			(tim_p->tm_hour < 12)  ?  CQ('A') :  CQ('P'));
	    if (len < 0  ||  (count+=len) >= maxsize)  return 0;
	  }
	  break;
	case CQ('R'):
          len = snprintf (&s[count], maxsize - count, CQ("%.2d:%.2d"),
			tim_p->tm_hour, tim_p->tm_min);
          if (len < 0  ||  (count+=len) >= maxsize)  return 0;
          break;
	case CQ('S'):
	  len = snprintf (&s[count], maxsize - count, CQ("%.2d"),
			tim_p->tm_sec);
          if (len < 0  ||  (count+=len) >= maxsize)  return 0;
	  break;
	case CQ('t'):
	  if (count < maxsize - 1)
	    s[count++] = CQ('\t');
	  else
	    return 0;
	  break;
	case CQ('T'):
	case CQ('X'):
          len = snprintf (&s[count], maxsize - count, CQ("%.2d:%.2d:%.2d"),
			tim_p->tm_hour, tim_p->tm_min, tim_p->tm_sec);
          if (len < 0  ||  (count+=len) >= maxsize)  return 0;
          break;
	case CQ('u'):
          if (count < maxsize - 1)
            {
              if (tim_p->tm_wday == 0)
                s[count++] = CQ('7');
              else
                s[count++] = CQ('0') + tim_p->tm_wday;
            }
          else
            return 0;
          break;
	case CQ('U'):
	  len = snprintf (&s[count], maxsize - count, CQ("%.2d"),
		       (tim_p->tm_yday + 7 -
			tim_p->tm_wday) / 7);
          if (len < 0  ||  (count+=len) >= maxsize)  return 0;
	  break;
	case CQ('V'):
	  {
	    int adjust = iso_year_adjust (tim_p);
	    int wday = (tim_p->tm_wday) ? tim_p->tm_wday - 1 : 6;
	    int week = (tim_p->tm_yday + 10 - wday) / 7;
	    if (adjust > 0)
		week = 1;
	    else if (adjust < 0)
		week = 52 + (4 >= (wday - tim_p->tm_yday
				   - isleap (tim_p->tm_year
					     + (YEAR_BASE - 1
						- (tim_p->tm_year < 0
						   ? 0 : 2000)))));
	    len = snprintf (&s[count], maxsize - count, CQ("%.2d"), week);
            if (len < 0  ||  (count+=len) >= maxsize)  return 0;
	  }
          break;
	case CQ('w'):
	  if (count < maxsize - 1)
            s[count++] = CQ('0') + tim_p->tm_wday;
	  else
	    return 0;
	  break;
	case CQ('W'):
	  {
	    int wday = (tim_p->tm_wday) ? tim_p->tm_wday - 1 : 6;
	    len = snprintf (&s[count], maxsize - count, CQ("%.2d"),
			(tim_p->tm_yday + 7 - wday) / 7);
            if (len < 0  ||  (count+=len) >= maxsize)  return 0;
	  }
	  break;
	case CQ('y'):
	    {
	      int year = tim_p->tm_year >= 0 ? tim_p->tm_year % 100
		: abs (tim_p->tm_year + YEAR_BASE) % 100;
	      len = snprintf (&s[count], maxsize - count, CQ("%.2d"), year);
              if (len < 0  ||  (count+=len) >= maxsize)  return 0;
	    }
	  break;
	case CQ('Y'):
	  if(tim_p->tm_year <= INT_MAX-YEAR_BASE)  {
	    len = snprintf (&s[count], maxsize - count, CQ("%04d"),
				tim_p->tm_year + YEAR_BASE);
	  }
	  else  {
	    register unsigned year;
	    year = (unsigned) tim_p->tm_year + (unsigned) YEAR_BASE;
	    len = snprintf (&s[count], maxsize - count, CQ("%04u"),
				tim_p->tm_year + YEAR_BASE);
	  }
          if (len < 0  ||  (count+=len) >= maxsize)  return 0;
	  break;
	case CQ('z'):
          if (tim_p->tm_isdst >= 0)
            {
	      long offset;
	      __tzinfo_type *tz = __gettzinfo ();
	      TZ_LOCK;
	      offset = -tz->__tzrule[tim_p->tm_isdst > 0].offset;
	      TZ_UNLOCK;
	      len = snprintf (&s[count], maxsize - count, CQ("%+03ld%.2ld"),
			offset / SECSPERHOUR,
			labs (offset / SECSPERMIN) % 60L);
              if (len < 0  ||  (count+=len) >= maxsize)  return 0;
            }
          break;
	case CQ('Z'):
	  if (tim_p->tm_isdst >= 0)
	    {
	      int size;
	      TZ_LOCK;
	      size = strlen(_tzname[tim_p->tm_isdst > 0]);
	      for (i = 0; i < size; i++)
		{
		  if (count < maxsize - 1)
		    s[count++] = _tzname[tim_p->tm_isdst > 0][i];
		  else
		    {
		      TZ_UNLOCK;
		      return 0;
		    }
		}
	      TZ_UNLOCK;
	    }
	  break;
	case CQ('%'):
	  if (count < maxsize - 1)
	    s[count++] = CQ('%');
	  else
	    return 0;
	  break;
	}
      if (*format)
	format++;
      else
	break;
    }
  if (maxsize)
    s[count] = CQ('\0');

  return count;
}
#if defined(_REGRESSION_TEST)
#define OUTSIZE	256
 
struct test {
	CHAR  *fmt;	
	size_t  max;	
	size_t	ret;	
	CHAR  *out;	
	};
struct list {
	const struct tm  *tms;	
	const struct test *vec;	
	int  cnt;		
	};
 
const char  TZ[]="TZ=EST5EDT";

const struct tm  tm0 = {
	.tm_min 	= 53,
	.tm_hour	= 9,
	.tm_mday	= 30,
	.tm_mon 	= 11,
	.tm_year	= 108,
	.tm_wday	= 2,
	.tm_yday	= 364,
	.tm_isdst	= 0
	};
const struct test  Vec0[] = {
	#define EXP(s)	sizeof(s)/sizeof(CHAR)-1, s
	{ CQ("%a"), 3+1, EXP(CQ("Tue")) },
	{ CQ("%A"), 7+1, EXP(CQ("Tuesday")) },
	{ CQ("%b"), 3+1, EXP(CQ("Dec")) },
	{ CQ("%B"), 8+1, EXP(CQ("December")) },
	{ CQ("%c"), 24+1, EXP(CQ("Tue Dec 30 09:53:47 2008")) },
	{ CQ("%C"), 2+1, EXP(CQ("20")) },
	{ CQ("%d"), 2+1, EXP(CQ("30")) },
	{ CQ("%D"), 8+1, EXP(CQ("12/30/08")) },
	{ CQ("%e"), 2+1, EXP(CQ("30")) },
	{ CQ("%F"), 10+1, EXP(CQ("2008-12-30")) },
	{ CQ("%g"), 2+1, EXP(CQ("09")) },
	{ CQ("%G"), 4+1, EXP(CQ("2009")) },
	{ CQ("%h"), 3+1, EXP(CQ("Dec")) },
	{ CQ("%H"), 2+1, EXP(CQ("09")) },
	{ CQ("%I"), 2+1, EXP(CQ("09")) },
	{ CQ("%j"), 3+1, EXP(CQ("365")) },
	{ CQ("%k"), 2+1, EXP(CQ(" 9")) },
	{ CQ("%l"), 2+1, EXP(CQ(" 9")) },
	{ CQ("%m"), 2+1, EXP(CQ("12")) },
	{ CQ("%M"), 2+1, EXP(CQ("53")) },
	{ CQ("%n"), 1+1, EXP(CQ("\n")) },
	{ CQ("%p"), 2+1, EXP(CQ("AM")) },
	{ CQ("%r"), 11+1, EXP(CQ("09:53:47 AM")) },
	{ CQ("%R"), 5+1, EXP(CQ("09:53")) },
	{ CQ("%S"), 2+1, EXP(CQ("47")) },
	{ CQ("%t"), 1+1, EXP(CQ("\t")) },
	{ CQ("%T"), 8+1, EXP(CQ("09:53:47")) },
	{ CQ("%u"), 1+1, EXP(CQ("2")) },
	{ CQ("%U"), 2+1, EXP(CQ("52")) },
	{ CQ("%V"), 2+1, EXP(CQ("01")) },
	{ CQ("%w"), 1+1, EXP(CQ("2")) },
	{ CQ("%W"), 2+1, EXP(CQ("52")) },
	{ CQ("%x"), 8+1, EXP(CQ("12/30/08")) },
	{ CQ("%X"), 8+1, EXP(CQ("09:53:47")) },
	{ CQ("%y"), 2+1, EXP(CQ("08")) },
	{ CQ("%Y"), 4+1, EXP(CQ("2008")) },
	{ CQ("%z"), 5+1, EXP(CQ("-0500")) },
	{ CQ("%Z"), 3+1, EXP(CQ("EST")) },
	{ CQ("%%"), 1+1, EXP(CQ("%")) },
	#undef EXP
	};
const struct tm  tm1 = {
	.tm_sec 	= 13,
	.tm_min 	= 1,
	.tm_hour	= 23,
	.tm_mday	= 2,
	.tm_mon 	= 6,
	.tm_year	= 108,
	.tm_wday	= 3,
	.tm_yday	= 183,
	.tm_isdst	= 1
	};
const struct test  Vec1[] = {
	#define EXP(s)	sizeof(s)/sizeof(CHAR)-1, s
	{ CQ("%a"), 3+1, EXP(CQ("Wed")) },
	{ CQ("%A"), 9+1, EXP(CQ("Wednesday")) },
	{ CQ("%b"), 3+1, EXP(CQ("Jul")) },
	{ CQ("%B"), 4+1, EXP(CQ("July")) },
	{ CQ("%c"), 24+1, EXP(CQ("Wed Jul  2 23:01:13 2008")) },
	{ CQ("%C"), 2+1, EXP(CQ("20")) },
	{ CQ("%d"), 2+1, EXP(CQ("02")) },
	{ CQ("%D"), 8+1, EXP(CQ("07/02/08")) },
	{ CQ("%e"), 2+1, EXP(CQ(" 2")) },
	{ CQ("%F"), 10+1, EXP(CQ("2008-07-02")) },
	{ CQ("%g"), 2+1, EXP(CQ("08")) },
	{ CQ("%G"), 4+1, EXP(CQ("2008")) },
	{ CQ("%h"), 3+1, EXP(CQ("Jul")) },
	{ CQ("%H"), 2+1, EXP(CQ("23")) },
	{ CQ("%I"), 2+1, EXP(CQ("11")) },
	{ CQ("%j"), 3+1, EXP(CQ("184")) },
	{ CQ("%k"), 2+1, EXP(CQ("23")) },
	{ CQ("%l"), 2+1, EXP(CQ("11")) },
	{ CQ("%m"), 2+1, EXP(CQ("07")) },
	{ CQ("%M"), 2+1, EXP(CQ("01")) },
	{ CQ("%n"), 1+1, EXP(CQ("\n")) },
	{ CQ("%p"), 2+1, EXP(CQ("PM")) },
	{ CQ("%r"), 11+1, EXP(CQ("11:01:13 PM")) },
	{ CQ("%R"), 5+1, EXP(CQ("23:01")) },
	{ CQ("%S"), 2+1, EXP(CQ("13")) },
	{ CQ("%t"), 1+1, EXP(CQ("\t")) },
	{ CQ("%T"), 8+1, EXP(CQ("23:01:13")) },
	{ CQ("%u"), 1+1, EXP(CQ("3")) },
	{ CQ("%U"), 2+1, EXP(CQ("26")) },
	{ CQ("%V"), 2+1, EXP(CQ("27")) },
	{ CQ("%w"), 1+1, EXP(CQ("3")) },
	{ CQ("%W"), 2+1, EXP(CQ("26")) },
	{ CQ("%x"), 8+1, EXP(CQ("07/02/08")) },
	{ CQ("%X"), 8+1, EXP(CQ("23:01:13")) },
	{ CQ("%y"), 2+1, EXP(CQ("08")) },
	{ CQ("%Y"), 4+1, EXP(CQ("2008")) },
	{ CQ("%z"), 5+1, EXP(CQ("-0400")) },
	{ CQ("%Z"), 3+1, EXP(CQ("EDT")) },
	{ CQ("%%"), 1+1, EXP(CQ("%")) },
	#undef EXP
	#define VEC(s)	s, sizeof(s)/sizeof(CHAR), sizeof(s)/sizeof(CHAR)-1, s
	#define EXP(s)	sizeof(s)/sizeof(CHAR), sizeof(s)/sizeof(CHAR)-1, s
	{ VEC(CQ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz")) },
	{ CQ("0123456789%%%h:`~"), EXP(CQ("0123456789%Jul:`~")) },
	{ CQ("%R%h:`~ %x %w"), EXP(CQ("23:01Jul:`~ 07/02/08 3")) },
	#undef VEC
	#undef EXP
	};
 
#if YEAR_BASE == 1900 
	.tm_sec 	= 13,
	.tm_min 	= 1,
	.tm_hour	= 23,
	.tm_mday	= 2,
	.tm_mon 	= 6,
	.tm_year	= INT_MAX - YEAR_BASE/2,
	.tm_wday	= 3,
	.tm_yday	= 183,
	.tm_isdst	= 1
	};
#if INT_MAX == 32767
#  define YEAR	CQ("33717")		
#  define CENT	CQ("337")
#  define Year	   CQ("17")
# elif INT_MAX == 2147483647
#  define YEAR	CQ("2147484597")
#  define CENT	CQ("21474845")
#  define Year	        CQ("97")
# elif INT_MAX == 9223372036854775807
#  define YEAR	CQ("9223372036854776757")
#  define CENT	CQ("92233720368547777")
#  define Year	                 CQ("57")
# else
#  error "Unrecognized INT_MAX value:  enhance me to recognize what you have"
#endif
const struct test  Vecyr0[] = {
	#define EXP(s)	sizeof(s)/sizeof(CHAR)-1, s
	{ CQ("%C"), OUTSIZE, EXP(CENT) },
	{ CQ("%c"), OUTSIZE, EXP(CQ("Wed Jul  2 23:01:13 ")YEAR) },
	{ CQ("%D"), OUTSIZE, EXP(CQ("07/02/")Year) },
	{ CQ("%F"), OUTSIZE, EXP(YEAR CQ("-07-02")) },
	{ CQ("%x"), OUTSIZE, EXP(CQ("07/02/")Year) },
	{ CQ("%y"), OUTSIZE, EXP(Year) },
	{ CQ("%Y"), OUTSIZE, EXP(YEAR) },
	#undef EXP
	};
#undef YEAR
#undef CENT
#undef Year
const struct tm  tmyr1 = {
	.tm_sec 	= 13,
	.tm_min 	= 1,
	.tm_hour	= 23,
	.tm_mday	= 2,
	.tm_mon 	= 6,
	.tm_year	= INT_MIN,
	.tm_wday	= 3,
	.tm_yday	= 183,
	.tm_isdst	= 1
	};
#if INT_MAX == 32767
#  define YEAR	CQ("-30868")		
#  define CENT	CQ("-308")
#  define Year	    CQ("68")
# elif INT_MAX == 2147483647
#  define YEAR	CQ("-2147481748")
#  define CENT	CQ("-21474817")
#  define Year	         CQ("48")
# elif INT_MAX == 9223372036854775807
#  define YEAR	CQ("-9223372036854773908")
#  define CENT	CQ("-92233720368547739")
#  define Year	                  CQ("08")
# else
#  error "Unrecognized INT_MAX value:  enhance me to recognize what you have"
#endif
const struct test  Vecyr1[] = {
	#define EXP(s)	sizeof(s)/sizeof(CHAR)-1, s
	{ CQ("%C"), OUTSIZE, EXP(CENT) },
	{ CQ("%c"), OUTSIZE, EXP(CQ("Wed Jul  2 23:01:13 ")YEAR) },
	{ CQ("%D"), OUTSIZE, EXP(CQ("07/02/")Year) },
	{ CQ("%F"), OUTSIZE, EXP(YEAR CQ("-07-02")) },
	{ CQ("%x"), OUTSIZE, EXP(CQ("07/02/")Year) },
	{ CQ("%y"), OUTSIZE, EXP(Year) },
	{ CQ("%Y"), OUTSIZE, EXP(YEAR) },
	#undef EXP
	};
#undef YEAR
#undef CENT
#undef Year
#endif /* YEAR_BASE ) */
const struct tm  tmyrzp = {

	.tm_sec 	= 60,
	.tm_min 	= 1,
	.tm_hour	= 23,
	.tm_mday	= 2,
	.tm_mon 	= 6,
	.tm_year	= 7-YEAR_BASE,
	.tm_wday	= 3,
	.tm_yday	= 183,
	.tm_isdst	= 1
	};
#define YEAR	CQ("0007")
#define CENT	CQ("00")
#define Year	  CQ("07")
const struct test  Vecyrzp[] = {
	#define EXP(s)	sizeof(s)/sizeof(CHAR)-1, s
	{ CQ("%C"), OUTSIZE, EXP(CENT) },
	{ CQ("%c"), OUTSIZE, EXP(CQ("Wed Jul  2 23:01:60 ")YEAR) },
	{ CQ("%D"), OUTSIZE, EXP(CQ("07/02/")Year) },
	{ CQ("%F"), OUTSIZE, EXP(YEAR CQ("-07-02")) },
	{ CQ("%x"), OUTSIZE, EXP(CQ("07/02/")Year) },
	{ CQ("%y"), OUTSIZE, EXP(Year) },
	{ CQ("%Y"), OUTSIZE, EXP(YEAR) },
	#undef EXP
	};
#undef YEAR
#undef CENT
#undef Year
const struct tm  tmyrzn = {
	.tm_sec 	= 00,
	.tm_min 	= 1,
	.tm_hour	= 23,
	.tm_mday	= 2,
	.tm_mon 	= 6,
	.tm_year	= -4-YEAR_BASE,
	.tm_wday	= 3,
	.tm_yday	= 183,
	.tm_isdst	= 1
	};
#define YEAR	CQ("-004")
#define CENT	CQ("-0")
#define Year	  CQ("04")
const struct test  Vecyrzn[] = {
	#define EXP(s)	sizeof(s)/sizeof(CHAR)-1, s
	{ CQ("%C"), OUTSIZE, EXP(CENT) },
	{ CQ("%c"), OUTSIZE, EXP(CQ("Wed Jul  2 23:01:00 ")YEAR) },
	{ CQ("%D"), OUTSIZE, EXP(CQ("07/02/")Year) },
	{ CQ("%F"), OUTSIZE, EXP(YEAR CQ("-07-02")) },
	{ CQ("%x"), OUTSIZE, EXP(CQ("07/02/")Year) },
	{ CQ("%y"), OUTSIZE, EXP(Year) },
	{ CQ("%Y"), OUTSIZE, EXP(YEAR) },
	#undef EXP
	};
#undef YEAR
#undef CENT
#undef Year
 
const struct list  ListYr[] = {
	{ &tmyrzp, Vecyrzp, sizeof(Vecyrzp)/sizeof(Vecyrzp[0]) },
	{ &tmyrzn, Vecyrzn, sizeof(Vecyrzn)/sizeof(Vecyrzn[0]) },
	#if YEAR_BASE == 1900
	{ &tmyr0, Vecyr0, sizeof(Vecyr0)/sizeof(Vecyr0[0]) },
	{ &tmyr1, Vecyr1, sizeof(Vecyr1)/sizeof(Vecyr1[0]) },
	#endif
	};/
const struct list  List[] = {
	{ &tm0, Vec0, sizeof(Vec0)/sizeof(Vec0[0]) },
	{ &tm1, Vec1, sizeof(Vec1)/sizeof(Vec1[0]) },
	};
 
#if defined(STUB_getenv_r)
char *
_getenv_r(struct _reent *p, const char *cp) { return getenv(cp); }
#endif
 
int
main(void)
{
int  i, l, errr=0, erro=0, tot=0;
const char  *cp;
CHAR  out[OUTSIZE];
size_t  ret;
cp = TZ;
if((i=putenv(cp)))  {
    printf( "putenv(%s) FAILED, ret %d\n", cp, i);
    return(-1);
    }
if(strcmp(getenv("TZ"),strchr(TZ,'=')+1))  {
    printf( "TZ not set properly in environment\n");
    return(-2);
    }
tzset();
 
#if defined(VERBOSE)
printf("_timezone=%d, _daylight=%d, _tzname[0]=%s, _tzname[1]=%s\n", _timezone, _daylight, _tzname[0], _tzname[1]);
{
long offset;
__tzinfo_type *tz = __gettzinfo ();
printf("tz->__tzrule[0].offset=%d, tz->__tzrule[1].offset=%d\n", tz->__tzrule[0].offset, tz->__tzrule[1].offset);
}
#endif
for(l=0; l<sizeof(List)/sizeof(List[0]); l++)  {
    const struct list  *test = &List[l];
    for(i=0; i<test->cnt; i++)  {
	tot++;
	ret = strftime(out, test->vec[i].max, test->vec[i].fmt, test->tms);
	if(ret != test->vec[i].ret)  {
	    errr++;
	    fprintf(stderr,
		"ERROR:  return %d != %d expected for List[%d].vec[%d]\n",
						ret, test->vec[i].ret, l, i);
	    }
	if(strncmp(out, test->vec[i].out, test->vec[i].max-1))  {
	    erro++;
	    fprintf(stderr,
		"ERROR:  \"%"SFLG"s\" != \"%"SFLG"s\" expected for List[%d].vec[%d]\n",
						out, test->vec[i].out, l, i);
	    }
	}
    }
for(l=0; l<sizeof(List)/sizeof(List[0]); l++)  {
    const struct list  *test = &List[l];
    for(i=0; i<test->cnt; i++)  {
	tot++;
	ret = strftime(out, test->vec[i].max-1, test->vec[i].fmt, test->tms);
	if(ret != 0)  {
	    errr++;
	    fprintf(stderr,
		"ERROR:  return %d != %d expected for List[%d].vec[%d]\n",
						ret, 0, l, i);
	    }
	if(strncmp(out, test->vec[i].out, test->vec[i].max-1-1))  {
	    erro++;
	    fprintf(stderr,
		"ERROR:  \"%"SFLG"s\" != \"%"SFLG"s\" expected for List[%d].vec[%d]\n",
						out, test->vec[i].out, l, i);
	    }
	}
    }
for(l=0; l<sizeof(ListYr)/sizeof(ListYr[0]); l++)  {
    const struct list  *test = &ListYr[l];
    for(i=0; i<test->cnt; i++)  {
	tot++;
	ret = strftime(out, test->vec[i].max, test->vec[i].fmt, test->tms);
	if(ret != test->vec[i].ret)  {
	    errr++;
	    fprintf(stderr,
		"ERROR:  return %d != %d expected for ListYr[%d].vec[%d]\n",
						ret, test->vec[i].ret, l, i);
	    }
	if(strncmp(out, test->vec[i].out, test->vec[i].max-1))  {
	    erro++;
	    fprintf(stderr,
		"ERROR:  \"%"SFLG"s\" != \"%"SFLG"s\" expected for ListYr[%d].vec[%d]\n",
						out, test->vec[i].out, l, i);
	    }
	}
    }
 
#define STRIZE(f)	#f
#define NAME(f)	STRIZE(f)
printf(NAME(strftime) "() test ");
if(errr || erro)  printf("FAILED %d/%d of", errr, erro);
  else    printf("passed");
printf(" %d test cases.\n", tot);
 
return(errr || erro);
}
#endif /* defined(_REGRESSION_TEST) ] */
