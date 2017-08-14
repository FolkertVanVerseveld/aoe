/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Genie game engine crash handler
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 *
 * Installs a signal handler for segmentation violations and
 * attempts to show crash info and end-user-friendly information.
 */

#define _GNU_SOURCE
#include "crash.h"
#include "prompt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#include <ucontext.h>

static volatile sig_atomic_t caught_signal = 0;

#define SEGV_TRACESZ 256

#if defined(__i386__) || defined(__x86_64__)
#define HAS_FAULT_ADDRESS 1
#else
#error unsupported architecture
#endif

#if NO_FAULT_ADDRESS
static void crash_handler(int sig)
{
	void *trace[SEGV_TRACESZ];
	size_t n;
	if (caught_signal) {
		fprintf(stderr, "panic: recaught signal %d\naborting\n", sig);
		abort();
	}
	caught_signal = 1;
	n = backtrace(trace, SEGV_TRACESZ);
	fprintf(stderr, "Fatal error: signal %d\nBacktrace:\n", sig);
	backtrace_symbols_fd(trace, n, STDERR_FILENO);
	char **list = backtrace_symbols(trace, SEGV_TRACESZ);
	char buf[4096];
	snprintf(
		buf, sizeof buf,
		"The application has crashed. Copy this text *exactly* if you want to report a bug.\n"
		"Debug info: got signal %s\nBacktrace:\n", strsignal(sig)
	);
	for (char *out = buf + strlen(buf), **trace = list; *trace && n; ++trace, --n) {
		int d = snprintf(out, (sizeof buf) - (out - buf), "%s\n", *trace);
		if (!d) break;
		out += d;
	}
	show_error("Fatal error", buf);
	exit(128 + (sig & 127));
}

int genie_crash_init(void)
{
	if (signal(SIGSEGV, crash_handler) == SIG_ERR)
		return 1;
	return 0;
}
#else

/* Must match struct ucontext in /usr/include/asm-generic/ucontext.h */
struct sig_ucontext {
	unsigned long     uc_flags;
	struct ucontext   *uc_link;
	stack_t           uc_stack;
	struct sigcontext uc_mcontext;
	sigset_t          uc_sigmask;
};

static void crash_handler(int sig, siginfo_t *info, void *ucontext)
{
	void *trace[SEGV_TRACESZ];
	void *caller;
	char **msg;
	int size, i;
	struct sig_ucontext *uc;

	if (caught_signal) {
		fprintf(stderr, "panic: recaught signal %d\naborting\n", sig);
		abort();
	}
	caught_signal = 1;
	uc = ucontext;
#if defined(__i386__)
	caller = (void*)uc->uc_mcontext.eip;
#elif defined(__x86_64__)
	caller = (void*)uc->uc_mcontext.rip;
#else
#error unsupported architecture
#endif
	size = backtrace(trace, 50);

	trace[1] = caller;
	msg = backtrace_symbols(trace, size);

	char buf[4096];
	snprintf(
		buf, sizeof buf,
		"The application has crashed. Copy this text *exactly* if you want to report a bug.\n"
		"Debug info: got %s\n"
		"Fault address: %p from %p (PID: %d)\nBacktrace:\n",
		strsignal(sig), info->si_addr, caller, getpid()
	);
	i = 0;
	for (char *out = buf + strlen(buf), **trace = msg; *trace && i < size; ++trace, ++i) {
		int d = snprintf(out, (sizeof buf) - (out - buf), "#%3d  %s\n", i + 1, *trace);
		if (!d)
			break;
		out += d;
	}
	show_error("Fatal error", buf);

	free(msg);
	exit(128 + (sig & 127));
}

int genie_crash_init(void)
{
	struct sigaction sigact;

	sigact.sa_sigaction = crash_handler;
	sigact.sa_flags = SA_RESTART | SA_SIGINFO;

	if (sigaction(SIGSEGV, &sigact, (struct sigaction *)NULL))
		return 1;
	return 0;
}
#endif
