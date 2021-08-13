#ifndef rsdef_h
#define rsdef_h

#ifndef M_PI
#define _USE_MATH_DEFINES
#endif
#include <math.h>

#ifdef M_PI
const double PI = M_PI;
#else
const double PI = 3.14159265358979323846;
#endif
const double NaN = std::nan(" ");

typedef unsigned char byte;
typedef unsigned long long uint64;

#endif
