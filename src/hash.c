/*
Copyright 2009 Aiko Barz

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

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "malloc.h"
#include "main.h"
#include "hash.h"

struct obj_hash *hash_init(unsigned int capacity ) {
	struct obj_hash *map = myalloc(sizeof(struct obj_hash), "hash_init");
	
	map->count = capacity;
	map->buckets = myalloc(map->count * sizeof(struct obj_bucket), "hash_init");

	return map;
}

void hash_free(struct obj_hash *map ) {
	unsigned int i;
	struct obj_bucket *bucket = NULL;

	if( map == NULL ) {
		return;
	}

	bucket = map->buckets;
	for( i=0; i<map->count; i++ ) {
		myfree(bucket->pairs, "hash_free");
		bucket++;
	}

	myfree(map->buckets, "hash_free");
	myfree(map, "hash_free");
	map = NULL;
}

int hash_exists(const struct obj_hash *map, UCHAR *key, long int keysize ) {
	if( map == NULL ) {
		return 0;
	}

	if( hash_get(map,key,keysize) != NULL ) {
		return 1;
	}
	
	return 0;
}

void *hash_get(const struct obj_hash *map, UCHAR *key, long int keysize ) {
	unsigned int index = 0;
	struct obj_bucket *bucket = NULL;
	struct obj_pair *pair = NULL;

	if( map == NULL || key == NULL ) {
		return NULL;
	}

	index = hash_this(key,keysize) % map->count;
	bucket = &(map->buckets[index]);
	pair = hash_getpair(bucket,key,keysize);

	if( pair == NULL ) {
		return NULL;
	}

	return pair->value;
}

int hash_put(struct obj_hash *map, UCHAR *key, long int keysize, void *value ) {
	unsigned int index = 0;
	struct obj_bucket *bucket = NULL;
	struct obj_pair *pair = NULL;

	if( map == NULL || key == NULL || value == NULL ) {
		return 0;
	}
	
	index = hash_this(key,keysize) % map->count;
	bucket = &(map->buckets[index]);
	
	/* Key already exists */
	if(( pair = hash_getpair(bucket, key, keysize)) != NULL ) {
		pair->value = value;
		return 1;
	}

	/* Create new obj_pair */
	if( bucket->count == 0 ) {
		bucket->pairs = myalloc(sizeof(struct obj_pair), "hash_put");
		bucket->count = 1;
	} else  {
		bucket->pairs = myrealloc(bucket->pairs,( bucket->count + 1) * sizeof(struct obj_pair), "hash_put");
		bucket->count++;
	}
	
	/* Store key pairs */
	pair = &(bucket->pairs[bucket->count - 1]);
	pair->key = key;
	pair->keysize = keysize;
	pair->value = value;
	
	return 1;
}

void hash_del(struct obj_hash *map, UCHAR *key, long int keysize ) {
	unsigned int index = 0;
	struct obj_bucket *bucket = NULL;
	struct obj_pair *thispair = NULL;
	struct obj_pair *oldpair = NULL;
	struct obj_pair *newpair = NULL;
	struct obj_pair *p_old = NULL;
	struct obj_pair *p_new = NULL;
	unsigned int i = 0;

	if( map == NULL || key == NULL ) {
		return;
	}
	
	/* Compute bucket */
	index = hash_this(key,keysize) % map->count;
	bucket = &(map->buckets[index]);

	/* Not found */
	if(( thispair = hash_getpair(bucket,key,keysize)) == NULL ) {
		return;
	}
	
	if( bucket->count == 1 ) {
		myfree(bucket->pairs, "hash_rem");
		bucket->pairs = NULL;
		bucket->count = 0;
	} else if( bucket->count > 1 ) {
		/* Get new memory and remember the old one */
		oldpair = bucket->pairs;
		newpair = myalloc((bucket->count - 1) * sizeof(struct obj_pair), "hash_rem");

		/* Copy pairs except the one to delete */
		p_old = oldpair;
		p_new = newpair;
		for( i=0; i<bucket->count; i++ ) {
			if( p_old != thispair ) {
				memcpy(p_new++, p_old, sizeof(struct obj_pair));
			}

			p_old++;
		}

		myfree(oldpair, "hash_rem");
		bucket->pairs = newpair;
		bucket->count--;
	}
}

struct obj_pair *hash_getpair(struct obj_bucket *bucket, UCHAR *key, long int keysize ) {
	unsigned int i;
	struct obj_pair *pair = NULL;

	if( bucket->count == 0 ) {
		return NULL;
	}

	pair = bucket->pairs;
	for( i=0; i<bucket->count; i++ ) {
		if( pair->keysize == keysize ) {
			if( pair->key != NULL && pair->value != NULL ) {
				if( memcmp(pair->key, key, keysize) == 0 ) {
					return pair;
				}
			}
		}
		pair++;
	}
	
	return NULL;
}

unsigned long hash_this(UCHAR *p, long int keysize ) {
	unsigned long result = 5381;
	long int i = 0;

	for( i=0; i<keysize; i++ ) {
		result = ((result << 5) + result) + *(p++);
	}

	return result;
}
