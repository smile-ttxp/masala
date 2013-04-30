/*
Copyright 2006 Aiko Barz

This file is part of masala/tumbleweed.

masala/tumbleweed is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala/tumbleweed is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala/tumbleweed.  If not, see <http://www.gnu.org/licenses/>.
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
#include <stdarg.h>

#ifdef TUMBLEWEED
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
#endif

#ifdef TUMBLEWEED
void log_complex( NODE *n, int code, const char *buffer ) {
	int verbosity = (_main->conf->quiet == CONF_BEQUIET && code == 200) ? CONF_BEQUIET : CONF_VERBOSE;
	char buf[INET6_ADDRSTRLEN+1];

	if( _main->nodes == NULL ) {
		return;
	}

	if( verbosity != CONF_VERBOSE ) {
		return;
	}

	memset( buf, '\0', INET6_ADDRSTRLEN+1 );
	
	if( _main->conf->mode == CONF_FOREGROUND ) {
		printf( "%.3li %.3u %s %s\n",
			_main->nodes->list->counter, code,
			inet_ntop( AF_INET6, &n->c_addr.sin6_addr, buf, INET6_ADDRSTRLEN ),
			buffer );
	} else {
		openlog( CONF_SRVNAME, LOG_PID|LOG_CONS,LOG_USER );
		syslog( LOG_INFO, "%.3li %.3u %s %s",
			_main->nodes->list->counter, code,
			inet_ntop( AF_INET6, &n->c_addr.sin6_addr, buf, INET6_ADDRSTRLEN ),
			buffer );
		closelog();
	}
}

void log_info( int code, const char *buffer ) {
	int verbosity = (_main->conf->quiet == CONF_BEQUIET && code == 200) ? CONF_BEQUIET : CONF_VERBOSE;

	if( _main->nodes == NULL ) {
		return;
	}

	if( verbosity != CONF_VERBOSE ) {
		return;
	}

	if( _main->conf->mode == CONF_FOREGROUND ) {
		printf( "%.3li %.3u ::1 %s\n", _main->nodes->list->counter,
			code, buffer );
	} else {
		openlog( CONF_SRVNAME, LOG_PID|LOG_CONS,LOG_USER );
		syslog( LOG_INFO, "%.3li %.3u ::1 %s", _main->nodes->list->counter,
			code, buffer );
		closelog();
	}
}
#endif


char* id_str( const UCHAR *in, char *buf ) {
	long int i = 0;
	UCHAR *p0 = (UCHAR *)in;
	char *p1 = buf;

	memset( buf, '\0', HEX_LEN+1 );

	for( i=0; i<SHA_DIGEST_LENGTH; i++ ) {
		snprintf( p1, 3, "%02x", *p0 );
		p0++;
		p1+=2;
	}

	return buf;
}

char* addr_str( IP *addr, char *addrbuf ) {
	char buf[INET6_ADDRSTRLEN+1];
	unsigned short port = ntohs( addr->sin6_port );

	inet_ntop( AF_INET6, &addr->sin6_addr, buf, sizeof(buf) );
	sprintf(addrbuf, "[%s]:%d", buf, port);

	return addrbuf;
}

void _log( const char *filename, int line, int priority, const char *format, ... ) {
	char buffer[MAIN_BUF];
	va_list vlist;

	va_start( vlist, format );
	vsnprintf( buffer, MAIN_BUF, format, vlist );
	va_end( vlist );

	if( priority == LOG_INFO && (_main->conf->quiet == CONF_BEQUIET) ) {
		return;
	}

	if( _main->conf->mode == CONF_FOREGROUND ) {
		fprintf( stderr, "(%s:%d) %s\n", filename, line, buffer );
	} else {
		openlog( CONF_SRVNAME, LOG_PID|LOG_CONS, LOG_USER|LOG_PERROR );
		syslog( priority, "(%s:%d) %s" , filename, line, buffer );
		closelog();
	}

	if( priority == LOG_CRIT || priority == LOG_ERR )
		exit( 1 );
}
