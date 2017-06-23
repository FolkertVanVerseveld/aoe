#ifndef GENIE_ENGINE_H
#define GENIE_ENGINE_H

#define GE_INIT_LEGACY_OPTIONS 1

int ge_init(int argc, char **argv, const char *title, unsigned options);
int ge_main(void);

#endif
