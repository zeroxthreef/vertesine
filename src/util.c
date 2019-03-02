#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/time.h>


char *default_config = "\
; default configuration\n\
\n\
; all boolean values use a 1 or 0\n\
[vertesine]\n\
port = 8080\n\
chrooted = 0\n\
max_cache_mb = 200\n\
log_path = vertesine.log\n\
max_filechunk_mb = 5\n\
sleep = 400\n\
elem_per_page = 15\n\
\n\
; some specifics: the database id is only 0 - 15\n\
[database]\n\
database_id = 0\n\
port = 6379\n\
login_token_ttl = 864000\n\
login_token_length = 25\n\
";

vert_mime_t vert_mimes[] = {
	{"*", "application/octet-stream"},
	{"html", "text/html"},
	{"htm", "text/html"},
	{"htmpl", "text/html"},
	{"css", "text/css"},
	{"js", "text/javascript"},
	{"jpg", "image/jpeg"},
	{"jpeg", "image/jpeg"},
	{"png", "image/png"},
	{"bmp", "image/bmp"},
	{"gif", "image/gif"},
	{"apng", "image/apng"},
	{"webp", "image/webp"},
	{"mp3", "audio/mpeg"},
	{"ogg", "audio/ogg"},
	{"wav", "audio/wav"},
	{"webm", "audiom/web"},
	{"midi", "audio/midi"},
	{"mid", "audio/midi"},
	{"ogv", "video/ogv"},
	{"mp4", "video/mp4"},
	{"ttf", "font/ttf"}
	/*
	{"", ""},
	{"", ""},
	{"", ""},
	{"", ""},
	 * */
};



short vert_util_asprintf(char **string, const char *fmt, ...)
{
	va_list list;
	char *temp_string = NULL;
	char *oldstring = NULL;
	int size = 0;

	if(*string != NULL)
	{
		//free(*string);
    	oldstring = *string;
	}

	va_start(list, fmt);
	size = vsnprintf(temp_string, 0, fmt, list);
	va_end(list);
	va_start(list, fmt);

	if((temp_string = malloc(size + 1)) != NULL)
	{
    	if(vsnprintf(temp_string, size + 1, fmt, list) != -1)
    	{
    		*string = temp_string;
    		if(oldstring != NULL)
			{
				free(oldstring);
			}
    	return size;
    }
    else
    {
		*string = NULL;
		if(oldstring != NULL)
		{
			free(oldstring);
		}
			return -1;
		}
	}
	va_end(list);
}

void vert_util_safe_free(void *ptr)
{
	if(ptr != NULL)
	{
		free(ptr);
	}
}

uint8_t *vert_util_read_file(const char *location, unsigned long *size_ptr) /* not entirely fit for binary reading since it adds a null terminator to the end */
{
	FILE *f;
	unsigned long size;
	unsigned char *dataPtr;
	f = fopen(location, "rb");
	if(f == NULL)
	{
		fprintf(stderr,"Error cant find %s\n", location);
		return (unsigned char *)NULL;
	}

	fseek(f,0,SEEK_END);
	size = ftell(f);
	rewind(f);

	dataPtr = (unsigned char *)malloc(size + 1);

	fread(dataPtr, sizeof(unsigned char), size, f);

	if(dataPtr == NULL)
	{
		fprintf(stderr, "Outta memory\n");
		return (unsigned char *)NULL;
	}

	fclose(f);
	*size_ptr = size;

	dataPtr[size] = 0x00;

	return dataPtr;
}

uint8_t *vert_util_read_file_range(char *location, unsigned long *sizePtr, unsigned long *fullSizePtr, unsigned long offset, unsigned long askSize)
{
	FILE *f;
	unsigned long size;
	uint8_t *dataPtr;
	f = fopen(location, "rb");
	if(f == NULL){
		return (uint8_t *)NULL;
	}

	if(fullSizePtr != NULL)
	{
		fseek(f ,0, SEEK_END);
	  *fullSizePtr = ftell(f);
	  rewind(f);
	}


	if(offset)
	{
		fseek(f, 0, SEEK_END);
		if(askSize)
			size = askSize;
		else
			size = ftell(f) - offset;

		fseek(f, offset, SEEK_SET);
	}
	else
	{
		fseek(f, 0, SEEK_END);
		if(askSize)
			size = askSize;
		else
			size = ftell(f);
		rewind(f);
	}


	dataPtr = (uint8_t *)malloc(size);


	fread(dataPtr, sizeof(unsigned char), size, f);

	if(dataPtr == NULL){
		log_fatal("unable to complete file read operation");
		return (uint8_t *)NULL;
	}

	rewind(f);
	fclose(f);

	*sizePtr = size;
	return dataPtr;
}

int vert_util_write_file(char *path, uint8_t *data, unsigned long len)
{
	FILE *f;
	int ret = 0;

	f = fopen(path,"wb");
	
	if(f != NULL)
		ret = fwrite(data, sizeof(uint8_t), len, f);

	fclose(f);

	return ret;
}

void vert_util_write_default_config() /* if the config file is not present, then write it */
{
	FILE *temp;
	temp = fopen("vertesine.ini", "r");
	
	if(temp != NULL)
	{
		fclose(temp);
		log_debug("config file exists");
	}
	else
	{
		log_error("config does not exist. Writing the default");
		vert_util_write_file("vertesine.ini", default_config, strlen(default_config));
	}
}

char *vert_util_replace(char *original, char *find, char *replace)
{
	char *final = NULL, *begining = NULL;

	if(!replace)
		replace = "template replace error";
	
	if(original != NULL)
	if((begining = strstr(original, find)) != NULL)
	{
		final = calloc(1, (strlen(original) - strlen(find)) + strlen(replace) + 1);

		memmove(final, original, (begining - original));
		memmove(final + strlen(final), replace, strlen(replace));
		memmove(final + strlen(final), original + ((begining - original) + strlen(find)), strlen(original + ((begining - original) + strlen(find))));

		return final;
	}

	return NULL;
}

char *vert_util_replace_all(char *original, char *find, char *replace)
{
	char *final = NULL, *last = NULL;

	if(original != NULL)
		while((final = vert_util_replace(final ? final : original, find, replace)) != NULL)
		{
			if(last != NULL)
			free(last);

			last = final;
		}

	return last;
}

int vert_util_replace_buffer_all(char **original, char *find, char *replace)
{
	char *final = NULL;


	if(original != NULL && *original != NULL)
		if((final = vert_util_replace_all(*original, find, replace)) != NULL)
		{
			free(*original);
			*original = final;

			return 1;
		}

	return 0;
}

char *vert_util_strdup(char *original)
{
	char *duplicate = NULL;

	if(original != NULL)
		if((duplicate = calloc(strlen(original) + 1, sizeof(char))) != NULL)
			strcpy(duplicate, original);

	return duplicate;
}

char *vert_util_mime(char *file)
{
	int i;
	char *ptr = NULL;
	
	if((ptr = strrchr(file, '.')) != NULL)
		{
			for(i = 0; i < sizeof(vert_mimes) / sizeof(vert_mimes[0]); i++)
			{
				if(strcmp(ptr + 1, vert_mimes[i].ext) == 0)
				{
					/* return the mime */
					return vert_mimes[i].mime;
				}
			}
			/* there was no recognized mime. return the default */
			return vert_mimes[0].mime;
		}
	
	
	
	return NULL;
}

double vert_util_get_ms()
{
	struct timeval ts;
	gettimeofday(&ts, NULL);
	
	return ((double)ts.tv_sec * 1000.0) + ((double)ts.tv_usec / 1000.0);
}

char *vert_util_get_page_dir(char *path)
{
	char *final_path = NULL, *temp_ptr = NULL, temp_char;
	
	if(strstr(path, ".htmpl")) /* the path ends with the index selected */
	{
		if(!(temp_ptr = strrchr(path, '/')))
		{
			log_error("path not valid");
			return NULL;
		}
		temp_char = *temp_ptr;
		*temp_ptr = 0x00;
		
		vert_util_asprintf(&final_path, "html%s/", path);
		
		*temp_ptr = temp_char;
	}
	else if(path[strlen(path) - 1] == '/') /* path ends with a trailing forward slash */
	{
		vert_util_asprintf(&final_path, "html%s", path);
	}
	else /* path ends without a trailing forward slash */
	{
		vert_util_asprintf(&final_path, "html%s/", path);
	}
	
	return final_path;
}
/*
char *vert_util_astrcat(char **strto, char *strfrom)
{
	char *returnstr;
	
	
	
	return returnstr;
}
*/