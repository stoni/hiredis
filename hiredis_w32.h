#ifndef __HIREDIS_W32_H__
#define __HIREDIS_W32_H__

#include <windows.h>
#include <time.h>

#include "sds.h"

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

#define strncasecmp strnicmp
#define strcasecmp stricmp

/**
 *
 */
struct timezone 
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};

/**
 *
 *
 */
static int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  unsigned __int64 tmpres = 0;
  static int tzflag;
 
  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);
 
    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;
 
    /*converting file time to unix epoch*/
    tmpres /= 10;  /*convert into microseconds*/
	tmpres -= DELTA_EPOCH_IN_MICROSECS; 
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }
 
  if (NULL != tz)
  {
    if (!tzflag)
    {
      _tzset();
      tzflag++;
    }
	tz->tz_minuteswest = _timezone / 60;
    tz->tz_dsttime = _daylight;
  }
 
  return 0;
}
/**
 *
 *
 */
static int inet_aton(const char *address, struct in_addr *sock)
{
    
    int s;
    s = inet_addr(address);
	if (s == INADDR_NONE) {
		return(0);
	}
    sock->s_addr = s;
    return(1);
}
/**
 *
 *
 */
static sds get_last_wsock_err()
{
	sds msg = NULL;
	char* buf = NULL;
	LPTSTR lpMsgBuf;
    unsigned long last_err = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        last_err,
        0,
        &lpMsgBuf,
        0, NULL
	);

	if (sizeof(TCHAR) == sizeof(char)) {
		size_t size = strlen(lpMsgBuf);
		buf = malloc(size + 1);
		strcpy(buf, lpMsgBuf);
	}
	else {
		size_t size = wcstombs(NULL, lpMsgBuf, 0);
		buf = malloc(size + 1);
		wcstombs(buf, lpMsgBuf, size + 1);
	}
	msg = sdsnew(buf);
	LocalFree(lpMsgBuf);
	free(buf);
	return(msg);
}

#endif