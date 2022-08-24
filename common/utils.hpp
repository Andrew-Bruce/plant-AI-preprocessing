#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#define Assert(x)	andyAssert(x, #x, __FILE__, __LINE__)


typedef unsigned int   UInt32;
typedef unsigned int   UInt;
typedef unsigned short UInt16;
typedef unsigned char  UInt8;

typedef signed int   SInt32;
typedef signed int   SInt;
typedef signed short SInt16;
typedef signed char  SInt8;

void
fatal(const char * const fmt, ...) __attribute__ ((format(printf, 1, 2)));

void
epf(const char * const fmt, ...)
  __attribute__ ((format(printf, 1, 2)))
  __attribute__ ((unused));



const char * syserr(void);

void
andyAssert(int x, const char *s, const char *file, int line);


void * Malloc(size_t n);

UInt8 *
Readfile(const char * const fn, UInt32 *size)
  __attribute__ ((unused));
