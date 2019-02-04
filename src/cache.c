#include "cache.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>

#define IN_BSIZE (1024 * (sizeof(struct inotify_event) + 16))


int inotify_fd;


int vert_filecache_init(unsigned int max_cachesize)
{
	map_init(&filecache);
	
	if((inotify_fd = inotify_init()) < 0)
	{
		log_fatal("unable to init inotify");
	}
	
	log_debug("filecache init");
	
	return 0;
}

int vert_filecache_destroy()
{
	map_deinit(&filecache);
	
	close(inotify_fd);
	
	log_debug("filecache destroyed");
	
	return 0;
}

uint8_t *vert_filecache_read(char *path, unsigned long *size_ptr, int type) /* only use for templates or small file. Adds a terminator */
{
	uint8_t *file = NULL;
	
	/* TODO make it work. Temporary non caches reading */
	if((file = vert_util_read_file(path, size_ptr)) != NULL)
	{
		if(type == VERT_CACHE_TEXT)
		{
			*size_ptr += 1;
	
			file = (uint8_t *)realloc(file, *size_ptr);
	
			file[*size_ptr - 1] = 0x00; /* add terminator */
		}
		
	}
	
	
	return file;
}

void vert_filecache_sendfile(char *path, void(*error_callback)(int error, sb_Event *e), sb_Event *e) /* for sending large and normal size files. Can do ranges */
{
	/* send a range of bytes if the file is over the maxfilechunk mb */
	/* TODO dont just wrap the provided function */
	unsigned long temp;
	uint8_t *temp_f = NULL;
	
	if(!(temp_f = vert_util_read_file(path, &temp)))
	{
		error_callback(404, e);
	}
	else if(sb_send_file(e->stream, path))
	{
		error_callback(500, e);
	}
	
	vert_util_safe_free(temp_f);
}

void vert_filecache_listen()
{
	
}