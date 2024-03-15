#ifdef _MSC_VER

#ifdef _WIN64
#include "opensslconf-msvc-x64.h"
#else
#include "opensslconf-msvc-x86.h"
#endif

#else

#ifdef _WIN64
#include "opensslconf-mingw-x64.h"
#else
#include "opensslconf-mingw-x86.h"
#endif

#endif
