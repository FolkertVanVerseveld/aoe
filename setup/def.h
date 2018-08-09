/* Copyright 2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Common used routines
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 */

#ifndef DEF_H
#define DEF_H

#ifdef __cplusplus
extern "C" {
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

void *fmalloc(size_t size);
void *frealloc(void *ptr, size_t size);

/* Paths to CDROM and wine installed directory */

// XXX http://insanecoding.blogspot.com/2007/11/pathmax-simply-isnt.html
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

extern char path_cdrom[PATH_MAX];
extern char path_wine[PATH_MAX];

#define WINE_PATH_FORMAT "/home/%s/.wine/drive_c/Program Files (x86)/Microsoft Games/Age of Empires"

extern int has_wine;
extern int game_installed;

void show_error(const char *str);
void panic(const char *str) __attribute__((noreturn));
void panicf(const char *format, ...) __attribute__((noreturn));

/* Wine auto detection */
int find_wine_installation(void);

/* CD-ROM/ISO auto detection */
int find_setup_files(void);

#ifdef __cplusplus
}
#endif

#endif
