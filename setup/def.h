/* Copyright 2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Common used routines
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 */

#ifndef DEF_H
#define DEF_H

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

void panic(const char *str) __attribute__((noreturn));

#endif
