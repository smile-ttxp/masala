/*
Copyright 2006 Aiko Barz

This file is part of masala.

masala is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

masala is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with masala.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/epoll.h>
#include <errno.h>
#include <pwd.h>
#include <fcntl.h>

#include "main.h"
#include "str.h"
#include "malloc.h"
#include "list.h"
#include "udp.h"
#include "log.h"
#include "file.h"
#include "conf.h"
#include "unix.h"
#include "hash.h"

void unix_signal( void ) {
	/* STRG+C aka SIGINT => Stop the program */
	_main->sig_stop.sa_handler = unix_sig_stop;
	_main->sig_stop.sa_flags = 0;
	if( ( sigemptyset( &_main->sig_stop.sa_mask) == -1) ||( sigaction( SIGINT, &_main->sig_stop, NULL) != 0) ) {
		log_err( "Failed to set SIGINT to handle Ctrl-C" );
	}

	/* ALARM */
	_main->sig_time.sa_handler = unix_sig_time;
	_main->sig_time.sa_flags = 0;
	if( ( sigemptyset( &_main->sig_time.sa_mask) == -1) ||( sigaction( SIGALRM, &_main->sig_time, NULL) != 0) ) {
		log_err( "Failed to set SIGINT to handle Ctrl-C" );
	}

	/* Ignore broken PIPE. Otherwise, the server dies too whenever a browser crashes. */
	signal( SIGPIPE,SIG_IGN );
}

void unix_sig_stop( int signo ) {
	_main->status = MAIN_SHUTDOWN;
	log_info( "Shutting down server" );
}

void unix_sig_time( int signo ) {
	_main->status = MAIN_SHUTDOWN;
}

void unix_set_time( int seconds ) {
	alarm( seconds );
}

void unix_fork( void ) {
	pid_t pid = 0;

	if( _main->conf->mode == CONF_FOREGROUND ) {
		return;
	}

	pid = fork();
	if( pid < 0 ) {
		log_err( "fork() failed" );
	} else if( pid != 0 ) {
	   exit( 0 );
	}

	/* Become session leader */
	setsid();
	
	/* Clear out the file mode creation mask */
	umask( 0 );
}

void unix_limits( void ) {
	struct rlimit rl;

	int guess = 2 * UDP_MAX_EVENTS * _main->conf->cores + 50;
	int limit = (guess < 4096) ? 4096 : guess; /* RLIM_INFINITY; */

	if( getuid() != 0 ) {
		return;
	}

	rl.rlim_cur = limit;
	rl.rlim_max = limit;

	if( setrlimit( RLIMIT_NOFILE, &rl) == -1 ) {
		log_err( strerror( errno) );
	}

	log_info( "Max open files: %i", limit );
}

void unix_write_pidfile( pid_t pid ) {
	char* pid_file = _main->conf->pid_file;

	if( pid_file == NULL )
		return;

	int fd = open( pid_file, O_WRONLY|O_CREAT|O_TRUNC, 0666 );
	if( fd < 0 ) {
		log_err( "open: Failed to open pid file." );
	}

	if( dprintf( fd, "%i", pid ) < 0 )
		log_err( "dprintf: Failed to write pid file." );

	if( close( fd ) < 0 )
		log_err( "close: Failed to close pid file." );
}

void unix_dropuid0( void ) {
	struct passwd *pw = NULL;

	if( getuid() != 0 ) {
		return;
	}

	/* Process is running as root, drop privileges */
	if( ( pw = getpwnam( _main->conf->user)) == NULL ) {
		log_err( "Dropping uid 0 failed. Use \"-u\" to set a valid user." );
	}
	if( setenv( "HOME", pw->pw_dir, 1) != 0 ) {
		log_err( "setenv: Setting new $HOME failed." );
	}
	if( setgid( pw->pw_gid) != 0 ) {
		log_err( "setgid: Unable to drop group privileges" );
	}
	if( setuid( pw->pw_uid) != 0 ) {
		log_err( "setuid: Unable to drop user privileges" );
	}

	/* Test permissions */
	if( setuid( 0 ) != -1 ) {
		log_err( "ERROR: Managed to regain root privileges?" );
	}
	if( setgid( 0 ) != -1 ) {
		log_err( "ERROR: Managed to regain root privileges?" );
	}

	log_info( "uid: %i, gid: %i( -u)", pw->pw_uid, pw->pw_gid );
}

void unix_environment( void ) {
#ifdef __i386__
	log_info( "Types: int: %i, long int: %i, size_t: %i, ssize_t: %i, time_t: %i", sizeof(int), sizeof(long int), sizeof(size_t), sizeof(ssize_t), sizeof(time_t) );
#else
	log_info( "Types: int: %lu, long int: %lu, size_t: %lu, ssize_t: %lu, time_t: %lu", sizeof(int), sizeof(long int), sizeof(size_t), sizeof(ssize_t), sizeof(time_t) );
#endif
}

int unix_cpus( void ) {
	return sysconf( _SC_NPROCESSORS_ONLN );
}
