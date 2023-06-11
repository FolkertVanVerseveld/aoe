/* fix annoying min/max macros by always undefining them. license: public domain/MIT */
#undef max
#undef min

#ifndef NOMINMAX_HPP
#define NOMINMAX_HPP

// make sure winntdef.h doesn't retry to define min/max
#define NOMINMAX 1

#include <algorithm>

#endif
