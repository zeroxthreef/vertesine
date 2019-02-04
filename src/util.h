#ifndef UTIL_H__
#define UTIL_H__

#include "../lib/vec/src/vec.h"
#include "../lib/ini/src/ini.h"
#include "../lib/map/src/map.h"
#include "../lib/log.c/src/log.h"
#include "../lib/sandbird/src/sandbird.h"
#include "../lib/snowflaked/src/snowflake.h"

#include <stdint.h>
#include <limits.h>

enum vert_states
{
	VERT_STATE_RUNNING,
	VERT_STATE_SHUTDOWN,
	VERT_STATE_UPDATING
};

typedef struct
{
	char *ext;
	char *mime;
}vert_mime_t;

map_int_t vert_settings;

FILE *log_file;


short vert_util_asprintf(char **string, const char *fmt, ...);

void vert_util_safe_free(void *ptr);

uint8_t *vert_util_read_file(const char *location, unsigned long *size_ptr);

uint8_t *vert_util_read_file_range(char *location, unsigned long *sizePtr, unsigned long *fullSizePtr, unsigned long offset, unsigned long askSize);

int vert_util_write_file(char *path, uint8_t *data, unsigned long len);

void vert_util_write_default_config();

char *vert_util_replace(char *original, char *find, char *replace);

char *vert_util_replace_all(char *original, char *find, char *replace);

int vert_util_replace_buffer_all(char **original, char *find, char *replace);

char *vert_util_strdup(char *original);

char *vert_util_mime(char *file);

double vert_util_get_ms();

char *vert_util_get_page_dir(char *path);

#endif