#include <_ansi.h>
#include "local.h"
#include <sys/lock.h>

#ifndef __SINGLE_THREAD__
__LOCK_INIT(static, __tz_lock_object);
#endif

_VOID
_DEFUN_VOID (__tz_lock)
{
#ifndef __SINGLE_THREAD__
  __lock_acquire(__tz_lock_object);
#endif
}

_VOID
_DEFUN_VOID (__tz_unlock)
{
#ifndef __SINGLE_THREAD__
  __lock_release(__tz_lock_object);
#endif
}
