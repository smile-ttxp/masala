/*
Copyright 2006 Aiko Barz

This file is part of masala/vinegar.

masala/vinegar is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala/vinegar is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala/vinegar.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/epoll.h>
#include <syslog.h>

#ifdef VINEGAR
#include "malloc.h"
#include "main.h"
#include "conf.h"
#include "str.h"
#include "thrd.h"
#include "list.h"
#include "node_web.h"
#include "log.h"
#else
#include "malloc.h"
#include "main.h"
#include "conf.h"
#include "str.h"
#include "thrd.h"
#include "list.h"
#include "log.h"
#include "hex.h"
#endif

#ifdef VINEGAR
void log_complex(struct obj_nodeItem *nodeItem, int code, const char *buffer ) {
	int verbosity = (_main->conf->quiet == CONF_BEQUIET && code == 200) ? CONF_BEQUIET : CONF_VERBOSE;
#ifndef IPV4
	char buf[INET6_ADDRSTRLEN+1];
#endif

	if( _main->node != NULL ) {
		if( verbosity == CONF_VERBOSE ) {
#ifndef IPV4
			memset(buf, '\0', INET6_ADDRSTRLEN+1);
#endif
			if( _main->conf->mode == CONF_FOREGROUND ) {
				printf("[%.3li] <%.3u> %s( %s)\n",
					_main->node->list->counter, code, buffer,
#ifdef IPV4
					inet_ntoa(nodeItem->c_addr.sin_addr)
#else
					inet_ntop(AF_INET6, &nodeItem->c_addr.sin6_addr, buf, INET6_ADDRSTRLEN)
#endif );
			} else {
				openlog(CONF_SRVNAME, LOG_PID|LOG_CONS,LOG_USER);
				syslog(LOG_INFO, "[%.3li] <%.3u> %s( %s)",
					_main->node->list->counter, code, buffer,
#ifdef IPV4
					inet_ntoa(nodeItem->c_addr.sin_addr)
#else
					inet_ntop(AF_INET6, &nodeItem->c_addr.sin6_addr, buf, INET6_ADDRSTRLEN)
#endif );
				closelog();
			}
		}
	}
}

void log_info(int code, const char *buffer ) {
	int verbosity = (_main->conf->quiet == CONF_BEQUIET && code == 200) ? CONF_BEQUIET : CONF_VERBOSE;
	
	if( _main->node != NULL ) {
		if( verbosity == CONF_VERBOSE ) {
			if( _main->conf->mode == CONF_FOREGROUND ) {
				printf("[%.3li] <%.3u> %s\n", _main->node->list->counter, code, buffer);
			} else {
				openlog(CONF_SRVNAME, LOG_PID|LOG_CONS,LOG_USER);
				syslog(LOG_INFO, "[%.3li] <%.3u> %s", _main->node->list->counter, code, buffer);
				closelog();
			}
		}
	}
}

#elif MASALA

void log_udp(CIPV6 *c_addr, const char *buffer ) {
	char v6buf[INET6_ADDRSTRLEN+1];
	
	int verbosity = (_main->conf->quiet == CONF_BEQUIET) ? CONF_BEQUIET : CONF_VERBOSE;

	if( verbosity == CONF_VERBOSE ) {
		memset(v6buf, '\0', INET6_ADDRSTRLEN+1);
		
		if( _main->conf->mode == CONF_FOREGROUND ) {
			printf("%s %s\n",
				buffer,
				inet_ntop(AF_INET6, &c_addr->sin6_addr, v6buf, INET6_ADDRSTRLEN) );
		} else {
			openlog(CONF_SRVNAME, LOG_PID|LOG_CONS,LOG_USER);
			syslog(LOG_INFO, "%s %s",
				buffer,
				inet_ntop(AF_INET6, &c_addr->sin6_addr, v6buf, INET6_ADDRSTRLEN) );
			closelog();
		}
	}
}

void log_info(const char *buffer ) {
	int verbosity = (_main->conf->quiet == CONF_BEQUIET) ? CONF_BEQUIET : CONF_VERBOSE;
	
	if( verbosity == CONF_VERBOSE ) {
		if( _main->conf->mode == CONF_FOREGROUND ) {
			printf("%s\n", buffer);
		} else {
			openlog(CONF_SRVNAME, LOG_PID|LOG_CONS,LOG_USER);
			syslog(LOG_INFO, "%s", buffer);
			closelog();
		}
	}
}
#endif

void log_simple(const char *buffer ) {
	printf("%s\n", buffer);
}

void log_fail(const char *buffer ) {
	if( _main->conf->mode == CONF_FOREGROUND ) {
		fprintf(stderr, "%s\n", buffer);
	} else {
		openlog(CONF_SRVNAME, LOG_PID|LOG_CONS,LOG_USER|LOG_PERROR);
		syslog(LOG_INFO, "%s", buffer);
		closelog();
	}
	exit(1);
}

void log_memfail(const char *buffer, const char *caller ) {
	if( _main->conf->mode == CONF_FOREGROUND ) {
		fprintf(stderr, "Memfail in %s: %s\n", caller, buffer);
	} else {
		openlog(CONF_SRVNAME, LOG_PID|LOG_CONS,LOG_USER|LOG_PERROR);
		syslog(LOG_INFO, "Memfail in %s: %s", caller, buffer);
		closelog();
	}
	exit(1);
}
