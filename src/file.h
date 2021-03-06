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

int file_isdir( const char *dirname );
int file_isreg( const char *filename );
int file_islink( const char *filename );
int file_mkdir( const char *dirname );

int file_rm( const char *filename );
int file_rmdir( const char *dirname );
int file_rmrf( char *filename );

size_t file_size( const char *filename );
time_t file_mod( const char *filename );

char *file_load( const char *filename, long int offset, size_t size );
int file_write( const char *filename, char *buffer, size_t size );
size_t file_append( const char *filename, char *buffer, size_t size );
