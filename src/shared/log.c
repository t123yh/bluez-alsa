/*
 * BlueALSA - log.c
 * Copyright (c) 2016-2018 Arkadiusz Bokowy
 *
 * This file is a part of bluez-alsa.
 *
 * This project is licensed under the terms of the MIT license.
 *
 */

#include "shared/log.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>

#include "shared/rt.h"


/* internal logging identifier */
static char *_ident = NULL;
/* if true, system logging is enabled */
static bool _syslog = false;
/* if true, print logging time */
static bool _time = BLUEALSA_LOGTIME;


void log_open(const char *ident, bool syslog, bool time) {

	free(_ident);
	_ident = strdup(ident);

	if ((_syslog = syslog) == true)
		openlog(ident, 0, LOG_USER);

	_time = time;

}

static void vlog(int priority, const char *format, va_list ap) {

	int oldstate;

	/* Threads cancellation is used extensively in the BlueALSA code. In order
	 * to prevent termination within the logging function (which might provide
	 * important information about what has happened), the thread cancellation
	 * has to be temporally disabled. */
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);

	if (_syslog) {
		va_list ap_syslog;
		va_copy(ap_syslog, ap);
		vsyslog(priority, format, ap_syslog);
		va_end(ap_syslog);
	}

	flockfile(stderr);

	if (_ident != NULL)
		fprintf(stderr, "%s: ", _ident);
	if (_time) {
		struct timespec ts;
		gettimestamp(&ts);
		fprintf(stderr, "%lu.%.9lu: ", (long int)ts.tv_sec, ts.tv_nsec);
	}
	vfprintf(stderr, format, ap);
	fputs("\n", stderr);

	funlockfile(stderr);

	pthread_setcancelstate(oldstate, NULL);

}

void error(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	vlog(LOG_ERR, format, ap);
	va_end(ap);
}

void warn(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	vlog(LOG_WARNING, format, ap);
	va_end(ap);
}

void info(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	vlog(LOG_INFO, format, ap);
	va_end(ap);
}

#if DEBUG
void _debug(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	vlog(LOG_DEBUG, format, ap);
	va_end(ap);
}
#endif

#if DEBUG
/**
 * Dump memory using hexadecimal representation.
 *
 * @param label Label printed before the memory block output.
 * @param mem Address of the memory block.
 * @param len Number of bytes which should be printed. */
void hexdump(const char *label, const void *mem, size_t len) {

	char *buf = malloc(len * 3 + 1);
	char *p = buf;

	while (len--) {
		p += sprintf(p, " %02x", *(unsigned char *)mem & 0xFF);
		mem += 1;
	}

	fprintf(stderr, "%s:%s\n", label, buf);
	free(buf);
}
#endif
