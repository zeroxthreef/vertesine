#ifndef CACHE_H__
#define CACHE_H__

#include "util.h"

enum vert_filecache_types
{
	VERT_CACHE_TEXT,
	VERT_CACHE_BINARY
};

typedef struct
{
	int fd;
	char *path;
	uint8_t *data;
	unsigned long total_size;
	unsigned long stored_size;
} vert_file_segment_t;

typedef map_t(vert_file_segment_t) file_map_t;

file_map_t filecache;


int vert_filecache_init(unsigned int max_cachesize);

int vert_filecache_destroy();

uint8_t *vert_filecache_read(char *path, unsigned long *size_ptr, int type); /* only use for templates or small files */

void vert_filecache_sendfile(char *path, void(*error_callback)(int error, sb_Event *e), sb_Event *e) ; /* for sending large and normal size files. Can do ranges */

void vert_filecache_listen();

#endif