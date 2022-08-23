#define Assert(x)	andyAssert(x, #x, __FILE__, __LINE__)


typedef unsigned int   UInt32;
typedef unsigned int   UInt;
typedef unsigned short UInt16;
typedef unsigned char  UInt8;

typedef signed int   SInt32;
typedef signed int   SInt;
typedef signed short SInt16;
typedef signed char  SInt8;

static __inline__ UInt min(UInt a, UInt b) { return (a < b) ? a : b; }

static void
fatal(const char * const fmt, ...) __attribute__ ((format(printf, 1, 2)));

static void
epf(const char * const fmt, ...)
  __attribute__ ((format(printf, 1, 2)))
  __attribute__ ((unused));

static void
epf(const char * const fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  return;
}

static void
fatal(const char * const fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
  exit(1);
}

static const char *
syserr(void)
{
  return strerror(errno);
}

static void
andyAssert(int x, const char *s, const char *file, int line)
{
  if (x == 0) {
    fatal("Assertion failed, %s line %d: %s\n", file, line, s);
  }
  return;
}

static void *
Malloc(size_t n)
{
  void *p = malloc(n);
  if (p == NULL) {
    fatal("Malloc(%d): %s", (int) n, syserr());
  }
  return p;
}

static UInt8 *
Readfile(const char * const fn, UInt32 *size)
__attribute__ ((unused));

static UInt8 *
Readfile(const char * const fn, UInt32 *size)
{
  struct stat statbuf;
  UInt32 filesize;
  UInt8 *data;
  SInt32 fd;

  if ((fd = open(fn, O_RDONLY)) < 0) {
    fatal("Readfile: Cannot open %s: %s", fn, syserr());
  }
  if (fstat(fd, &statbuf) != 0) {
    fatal("Cannot stat %s: %s", fn, syserr());
  }
  filesize = (size_t) statbuf.st_size;
  data = (UInt8 *) Malloc(filesize);
  if (read(fd, data, (size_t) filesize) != (ssize_t) filesize) {
    fatal("read error, %s: %s", fn, syserr());
  }
  close(fd);
  if (size != NULL) {
    *size = filesize;
  }
  return data;
}
