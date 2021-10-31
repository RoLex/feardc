// support includes from recent MS SDKs.

#define _MSC_VER 1400 // simulate a high enough version to make sure the include doesn't define junk

#define __in
#define __in_opt
#define _In_
#define _In_opt_

#include_next <htmlhelp.h>

#undef __in
#undef __in_opt
#undef _In_
#undef _In_opt_

#undef _MSC_VER
