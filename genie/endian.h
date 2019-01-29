/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info. */

/**
 * Simple byte endian order checking
 *
 * Licensed under MIT License.
 * Copyright 2019 Folkert van Verseveld
 */

#ifndef GENIE_ENDIAN_H
#define GENIE_ENDIAN_H

#ifndef _BSD_SOURCE
#define _BSD_SOURCE 1
#endif
#include <endian.h>

#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN
  #define GENIE_BYTE_ORDER_LITTLE 0
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN
  #define GENIE_BYTE_ORDER_LITTLE 1
#else
  #error "Unknown architecture"
#endif

#endif
