#ifndef OS_CHECK_H
#define OS_CHECK_H

#if defined(_WIN32) || defined(_WIN64) || (defined(__CYGWIN__) && !defined(_WIN32))
#define OS_WIN
#elif defined(__linux__)
#define OS_LIN
#endif

#endif
