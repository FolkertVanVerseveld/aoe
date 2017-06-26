/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef AOE_LOG_H
#define AOE_LOG_H

#include <time.h>

extern const char *logger_error_path;

struct logger {
	unsigned timestamp_enable;
	unsigned datetime_enable;
	unsigned write_log;
	unsigned write_stdout;
	unsigned sequence_number;
	unsigned flush;
	unsigned valid_log_opened;
	unsigned too_many_log_files;
	time_t time_millis;
	unsigned num24;
	char path[32];
};

struct logger *logger_init(struct logger *this);
/** print timestamp */
void logger_elapsed_ms(struct logger *this);
void logger_enable_timestamp(struct logger *this, int enable);
void logger_write_log(struct logger *this, int write);
void logger_write_stdout(struct logger *this, int write);
/** just close log file and stop logging */
int logger_free(struct logger *this);
int logger_stop(struct logger *this);

#endif
