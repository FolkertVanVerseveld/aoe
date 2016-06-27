/*
This file is reconstructed from debug information from the original game.
DO NOT modify strings constants in this file!
*/
#include "log.h"
#include "dbg.h"
#include "todo.h"

const char *logger_error_path = /*"C:\\"*/"APPLOGERR.TXT";

static inline const char *get_str_active(int is_active)
{
	return is_active ? " is now ACTIVE" : " has been deactivated";
}

void logger_enable_timestamp(struct logger *this, int enable)
{
	this->timestamp_enable = enable != 0;
	dbgf("Timestamp milliseconds %s\n", get_str_active(enable != 0));
}

static inline void logger_enable_datetime(struct logger *this, int enable)
{
	this->datetime_enable = enable != 0;
	dbgf("Date & Time stamp%s\n", get_str_active(this->timestamp_enable));
}

static inline void logger_set_flushing(struct logger *this, int flush)
{
	this->flush = flush == 1;
	dbgf("Flush queue each message%s\n", get_str_active(this->flush));
}

static inline void logger_sequence(struct logger *this, int sequence_number)
{
	if (sequence_number) {
		this->sequence_number = 1;
		this->num24 = 0;
	} else
		this->sequence_number = 0;
	dbgf("Sequence numbering%s\n", get_str_active(this->sequence_number));
}

static inline void logger_io_init(struct logger *this)
{
	stub
	if (this->write_log && (!this->valid_log_opened || this->too_many_log_files)) {
		time(&this->time_millis);
		for (int i = 0; i < 24; ++i) {
			// TODO look for log file
		}
	}
	this->valid_log_opened = 1;
	logger_elapsed_ms(this);
}

struct logger *logger_init(struct logger *this)
{
#ifndef STRICT
	// BUGFIX uninitialized
	time(&this->time_millis);
#endif
#ifdef DEBUG
	this->write_log = 0;
#else
	this->write_log = 0;
#endif
	this->write_stdout = 0;
	this->valid_log_opened = 0;
	this->too_many_log_files = 0;
	logger_sequence(this, 0);
	logger_enable_timestamp(this, 0);
	logger_enable_datetime(this, 0);
	logger_set_flushing(this, 0);
	logger_io_init(this);
	// FIXME call logger local func
	dbgs("===Logging===");
	logger_elapsed_ms(this);
	return this;
}

void logger_elapsed_ms(struct logger *this)
{
	time_t time_millis;
	time(&time_millis);
	struct tm *tm0 = localtime(&time_millis);
	float time_sec = (time_millis - this->time_millis) * 0.001;
	if (tm0) dbgf(">>>TIME>>>  %s  %7.1f sec elapsed\n", asctime(tm0), time_sec);
}

void logger_write_log(struct logger *this, int write)
{
	this->write_log = write == 1;
	printf("Logging to File %s\n", get_str_active(write));
}

void logger_write_stdout(struct logger *this, int write)
{
	this->write_log = write == 1;
	printf("Logging to OUTPUT %s\n", get_str_active(write));
}
