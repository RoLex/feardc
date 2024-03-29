#include "config.h.in"
#include <windef.h>

#if __MINGW64_VERSION_MAJOR < 8

#define HAVE_NONSTANDARD_PRINTF_64_FORMAT 1

#endif

#define HAVE_INTTYPES_H 1
#define HAVE_MEMORY_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#define STDC_HEADERS 1

#ifdef _WIN32

#define HAVE_WINDOWS_PATH 1

#else

#define HAVE_ELF_H 1

#endif
