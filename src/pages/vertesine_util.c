/* this utility file is for site specific datastructures */
#include "vertesine_util.h"
#include "../util.h"
#include "../templating.h"

#include <sodium.h>
#include <b64/cencode.h>
#include <gd.h>

#include <sys/time.h>


/* TODO create a user struct */

int vert_custom_init()
{
	struct timeval to = {2, 0};
	redisReply *reply;
	
	srand(time(NULL));
	
	log_info("connecting to db");
	if((vert_redis_ctx = redisConnectWithTimeout("127.0.0.1", *map_get(&vert_settings, "db_port"), to)) == NULL)
	{
		log_fatal("COULD NOT CONNECT TO DB!");
		redisFree(vert_redis_ctx);
		return 1;
	}
	/* TODO be prepared for db disconnects */
	
	/* TODO run the auth command if there is a password */
	
	reply = redisCommand(vert_redis_ctx, "PING");
	
	
	if(reply)
	{
		log_info("db ping response %s", reply->str);
		freeReplyObject(reply);
	}
	else
		log_error("db not responding");
	
	reply = redisCommand(vert_redis_ctx, "CLIENT SETNAME vertesine");
	
	
	if(reply)
	{
		log_info("db setname response %s", reply->str);
		freeReplyObject(reply);
	}
	
	/* select the db. THis is if multiple projects run on the same redis db and want to avoid clashing */
	
	log_info("selecting database db%d. Please make sure no other serive is using this index as to avoid clashing", *map_get(&vert_settings, "database_id"));
	
	reply = redisCommand(vert_redis_ctx, "SELECT %d", *map_get(&vert_settings, "database_id"));
	
	
	if(reply)
	{
		log_info("db select response %s", reply->str);
		freeReplyObject(reply);
	}
	
	sodium_init();
	
	/* a debug test TODO remove */
	uint64_t test_id = vert_custom_db_get_id("vertesine:comment:666");
	int test_type = vert_custom_db_get_type("vertesine:comment:666");
	
	log_trace("test id: %llu test type: %d", test_id, test_type);
	
	return 0;
}

void vert_custom_destroy()
{
	log_info("freeing the db context");
	
	if(vert_redis_ctx)
		redisFree(vert_redis_ctx);
}

char *vert_custom_generate_db_key(uint64_t id, int type)
{
	char *return_str = NULL;
	char *type_str = NULL;
	
	switch(type)
	{
		case VERT_CUSTOM_OBJECT_USER:
			type_str = "user";
		break;
		case VERT_CUSTOM_OBJECT_ENTRY:
			type_str = "entry";
		break;
		case VERT_CUSTOM_OBJECT_PROJECT:
			type_str = "project";
		break;
		case VERT_CUSTOM_OBJECT_COMMENT:
			type_str = "comment";
		break;
		case VERT_CUSTOM_OBJECT_UTOKEN:
			type_str = "user_token";
		break;
		default:
			type_str = "misc";
	}
	
	vert_util_asprintf(&return_str, "vertesine:%s:%lu", type_str, id);
	
	return return_str;
}

uint64_t vert_custom_db_get_id(char *db_key)
{
	uint64_t id = 0;
	char *ptr = NULL;
	
	if(strncmp(db_key, "vertesine:", strlen("vertesine:")) == 0)
	{
		if((ptr = strrchr(db_key, ':')))
		{
			id = strtol(ptr + 1, NULL, 10);
		}
	}
	
	return id;
}

int vert_custom_db_get_type(char *db_key)
{
	int type = -1, counter = 0;
	char *copy, *tok;
	
	if(strncmp(db_key, "vertesine:", strlen("vertesine:")) == 0)
	{
		copy = vert_util_strdup(db_key);
		tok = strtok(copy, ":");
		
		while(tok)
		{
			if(counter == 1)
			{
				
				if(strcmp(tok, "user") == 0)
				{
					type = VERT_CUSTOM_OBJECT_USER;
				}
				else if(strcmp(tok, "entry") == 0)
				{
					type = VERT_CUSTOM_OBJECT_ENTRY;
				}
				else if(strcmp(tok, "project") == 0)
				{
					type = VERT_CUSTOM_OBJECT_PROJECT;
				}
				else if(strcmp(tok, "comment") == 0)
				{
					type = VERT_CUSTOM_OBJECT_COMMENT;
				}
				else
				{
					type = VERT_CUSTOM_OBJECT_MISC;
				}
				
				break;
			}
			counter++;
			tok = strtok(NULL, ":");
		}
		
		vert_util_safe_free(copy);
	}
	
	return type;
}

char *vert_custom_random_string(unsigned long maxlen)
{
	char *final = NULL;
	unsigned char *temp_buf = NULL;
	base64_encodestate enc;
	int b64len = 0;

	if((final = calloc(1, 280 + maxlen)) == NULL)
	{
		fprintf(stderr, "couldnt calloc for random string\n");
		return NULL;
	}

	base64_init_encodestate(&enc);

	if((temp_buf = calloc(1, maxlen)) == NULL)
	{
		fprintf(stderr, "couldnt calloc for random string\n");
		return NULL;
	}

	randombytes_buf(temp_buf, maxlen); /* if only it didnt take so long */


	b64len = base64_encode_block(temp_buf, maxlen, final, &enc);

	final[b64len] = 0x00;

	vert_util_safe_free(temp_buf);


	return final;
}

void vert_custom_set_get_cookie(sb_Event *e, char *cookie_str, int send)
{
	static char *cookie = NULL;
	
	if(cookie_str)
	{
		vert_util_safe_free(cookie);
		cookie = vert_util_strdup(cookie_str);
	}
	
	if(send)
	{
		if(cookie)
		{
			log_trace("setting cookie");
			sb_send_header(e->stream, "Set-Cookie", cookie);
		
			vert_util_safe_free(cookie);
			cookie = NULL;
		}
		
	}
	
}

char *vert_custom_get_http_date(time_t time_num)
{
	struct tm tms = *gmtime(&time_num);
	char time_str[180];
	
	strftime(time_str, sizeof(time_str), "%a, %d %b %G %H:%M:%S GMT", &tms);
	
	
	return vert_util_strdup(time_str);
}

uint64_t vert_custom_logged_in(sb_Event *e)
{
	char cookie_buffer[202], *temp_str = NULL;
	redisReply *reply;
	
	
	cookie_buffer[0] = 0x00;
	
	sb_get_cookie(e->stream, "token", cookie_buffer, sizeof(cookie_buffer));
	
	if(strlen(cookie_buffer) > 10 && strlen(cookie_buffer) < 200)
	{
		temp_str = vert_custom_generate_db_key(strtol(cookie_buffer, NULL, 10), VERT_CUSTOM_OBJECT_UTOKEN);
		
		reply = redisCommand(vert_redis_ctx,"GET %s", temp_str);
		if(reply)
		{
			/* finally, compare the cookie */
			
			if(reply->type == REDIS_REPLY_STRING && strcmp(cookie_buffer, reply->str) == 0)
			{
				freeReplyObject(reply);
				vert_util_safe_free(temp_str);
				
				return strtol(cookie_buffer, NULL, 10);
			}
			
			vert_util_safe_free(temp_str);
			freeReplyObject(reply);
		}
	}
	return 0;
}

char *vert_custom_get_id_user_field(char *id, char *field)
{
	char *reply_str = NULL, *temp_str;
	redisReply *reply;
	
	
	temp_str = vert_custom_generate_db_key(strtol(id, NULL, 10), VERT_CUSTOM_OBJECT_USER);
	
	reply = redisCommand(vert_redis_ctx,"HGET %s %s", temp_str, field);
	if(reply)
	{
		/* finally, compare the cookie */
		if(reply->type != REDIS_REPLY_ERROR)
			reply_str = vert_util_strdup(reply->str);
		
		
		freeReplyObject(reply);
	}
	
	
	vert_util_safe_free(temp_str);
	
	return reply_str;
}

char *vert_custom_get_username_user_field(char *username, char *field)
{
	char *reply_str = NULL, *temp_str;
	redisReply *reply;
	int i;
	
	/* TODO loop through the list of usernames and if it exists, hget the user field */
	reply = redisCommand(vert_redis_ctx, "LRANGE vertesine:variable:users 0 -1");
	if(reply)
	{
		if(reply->type == REDIS_REPLY_ARRAY)
			for(i = 0; i < reply->elements; i++)
			{
				/* data is stored as "id:username" */
				temp_str = strchr(reply->element[i]->str, ':');
				temp_str++;
				if(strcmp(temp_str, username) == 0)
				{
					/* found user */
					temp_str--;
					*temp_str = 0x00;
					
					reply_str = vert_custom_get_id_user_field(reply->element[i]->str, field);
					
					break;
				}
			}
			
			freeReplyObject(reply);
	}
	
	
	
	return reply_str;
}

int vert_custom_check_user_exists(char *username)
{
	redisReply *reply;
	char *temp_str = NULL;
	int i, return_val = 0;
	
	reply = redisCommand(vert_redis_ctx, "LRANGE vertesine:variable:users 0 -1");
	if(reply)
	{
		if(reply->type == REDIS_REPLY_ARRAY)
			for(i = 0; i < reply->elements; i++)
			{
				/* data is stored as "id:username" */
				temp_str = strchr(reply->element[i]->str, ':');
				temp_str++;
				if(strcmp(temp_str, username) == 0)
				{
					/* found user */
					return_val = 1;
					
					break;
				}
			}
			
			freeReplyObject(reply);
	}
	
	return return_val;
}

uint32_t vert_custom_get_username_user_permissions(char *username)
{
	uint32_t perm = 0;
	char *ret = NULL;
	
	if((ret = vert_custom_get_username_user_field(username, "permissions")))
		perm = strtol(ret, NULL, 10);
	
	vert_util_safe_free(ret);
	
	return perm;
}

void vert_custom_filter_input(char **buffer, uint8_t depth)
{
	
	switch(depth)
	{
		case 1: /* filters out html < and > */
			vert_util_replace_buffer_all(buffer, "<", "&#60;");
			vert_util_replace_buffer_all(buffer, ">", "&#62;");
		default: /* filters out template tags */
			vert_util_replace_buffer_all(buffer, "[", "&#91;");
			vert_util_replace_buffer_all(buffer, "]", "&#93;");
	}
	
}

void vert_custom_filter_tags(char **buffer)
{
	char *key = NULL, *ptr = NULL;
	map_iter_t i = map_iter(&template_callbacks);
	
	while((key = map_next(&template_callbacks, &i)) != NULL)
	{
		if((ptr = strstr(*buffer, key)))
			vert_util_replace_buffer_all(buffer, key, "illegal_tag");
	}
	
}

uint8_t vert_custom_write_hash_field(uint64_t id, int object_type, char *field, char *data)
{
	uint8_t ret = 0;
	redisReply *reply;
	char *key;
	
	key = vert_custom_generate_db_key(id, object_type);
	
	reply = redisCommand(vert_redis_ctx, "HSET %s %s %s", key, field, data);
	if(reply)
	{
		if(reply->type == REDIS_REPLY_ERROR)
			ret++;
		
		freeReplyObject(reply);
	}
	
	vert_util_safe_free(key);
	
	return ret;
}

int vert_custom_manage_pfp(uint8_t *data, char *extension, char *user_id, unsigned long size, char **pfp_loc)
{
	gdImagePtr pfp;
	FILE *write;
	char *writepath = NULL, *urlpath = NULL;
	
	vert_util_asprintf(&writepath, "html/user_av/%s.%s", user_id, extension); /* 100% not letting the filenames be the username. Path traversal would suck */
	vert_util_asprintf(&urlpath, "/user_av/%s.%s", user_id, extension);
	
	if(strcmp(extension, "png") == 0)
	{
		pfp = gdImageCreateFromPngPtr((int)size, data);
		if(pfp == NULL)
			return 1;
		
		if(vert_custom_delete_usericon(user_id))
		{
			log_error("unable to delete old file");
			return 1;
		}
		
		/* write it to disk */
		if((write = fopen(writepath, "wb")) == NULL)
		{
			fprintf(stderr, "pfp image save error [%s]\n", writepath);
			gdImageDestroy(pfp);
			return 1;
		}
		
		gdImagePng(pfp, write); /* hopefully this removes all EXIF data to prevent people from uploading their coordinates or other sensitive info */
		gdImageDestroy(pfp);
		fclose(write);
		*pfp_loc = urlpath;
		free(writepath);
		
		return 0;
	}
	else if(strcmp(extension, "jpg") == 0 || strcmp(extension, "jpeg") == 0)
	{
		pfp = gdImageCreateFromJpegPtr((int)size, data);
		if(pfp == NULL)
		return 1;
		
		if(vert_custom_delete_usericon(user_id))
		{
			log_error("unable to delete old file");
			return 1;
		}
		
		/* write it to disk */
		if((write = fopen(writepath, "wb")) == NULL)
		{
			fprintf(stderr, "pfp image save error [%s]\n", writepath);
			gdImageDestroy(pfp);
			return 1;
		}
		
		gdImageJpeg(pfp, write, -1); /* hopefully this removes all EXIF data to prevent people from uploading their coordinates or other sensitive info */
		gdImageDestroy(pfp);
		fclose(write);
		*pfp_loc = urlpath;
		free(writepath);
		
		return 0;
	}
	else if(strcmp(extension, "gif") == 0)
	{
		pfp = gdImageCreateFromGifPtr((int)size, data);
		if(pfp == NULL)
		return 1;
		
		if(vert_custom_delete_usericon(user_id))
		{
			log_error("unable to delete old file");
			return 1;
		}
		
		/* write it to disk */
		if((write = fopen(writepath, "wb")) == NULL)
		{
			fprintf(stderr, "pfp image save error [%s]\n", writepath);
			gdImageDestroy(pfp);
			return 1;
		}
		
		/* gdImageGif(pfp, write); /* hopefully this removes all EXIF data to prevent people from uploading their coordinates or other sensitive info */
		fwrite(data, sizeof(unsigned char), size, write);
		gdImageDestroy(pfp);
		fclose(write);
		*pfp_loc = urlpath;
		free(writepath);
		
		return 0;
	}
	else if(strcmp(extension, "bmp") == 0)
	{
		pfp = gdImageCreateFromBmpPtr((int)size, data);
		if(pfp == NULL)
			return 1;
		
		if(vert_custom_delete_usericon(user_id))
		{
			log_error("unable to delete old file");
			return 1;
		}
		
		/* write it to disk */
		if((write = fopen(writepath, "wb")) == NULL)
		{
			fprintf(stderr, "pfp image save error [%s]\n", writepath);
			gdImageDestroy(pfp);
			return 1;
		}
		
		gdImageBmp(pfp, write, -1); /* hopefully this removes all EXIF data to prevent people from uploading their coordinates or other sensitive info */
		gdImageDestroy(pfp);
		fclose(write);
		*pfp_loc = urlpath;
		free(writepath);
		
		return 0;
	}
	
	
	return 1;
}

char *vert_custom_determine_extension(uint8_t *data, size_t data_len)
{
	uint8_t png_sig[] = {137, 80, 78, 71, 13, 10, 26, 10}, jpg_sig[] = {0xFF, 0xD8, 0xFF}, bmp_sig[] = {0x42, 0x4D}, gif_sig[] = {0x47, 0x49, 0x46, 0x38};
	
	if(data_len > 8)
	{
		if(memcmp(data, png_sig, sizeof(png_sig)) == 0)
		{
			return "png";
		}
		if(memcmp(data, jpg_sig, sizeof(jpg_sig)) == 0)
		{
			return "jpg";
		}
		if(memcmp(data, bmp_sig, sizeof(bmp_sig)) == 0)
		{
			return "bmp";
		}
		if(memcmp(data, gif_sig, sizeof(gif_sig)) == 0)
		{
			return "gif";
		}
	}
	
	return NULL;
}

char *vert_custom_generate_pagenum_nav(char *path, size_t current, size_t maximum)
{
	char *temp_str = NULL, *template = "%s<a href=\"%s?p=%d\"><div class=\"entries_pages_contrainer_numbers_%sactive\">%d</div></a>";
	size_t i = 1;
	
	do
	{
		/*log_trace("big debug - %lu %lu %lu %llu", i, i * *map_get(&vert_settings, "elem_per_page"), maximum, current); */
		vert_util_asprintf(&temp_str, template, temp_str ? temp_str : "", path, i, (current == i) ? "" : "in", i);
		
		i++;
	}while(i * *map_get(&vert_settings, "elem_per_page") <= maximum + *map_get(&vert_settings, "elem_per_page"));
	
	/*
	vert_util_asprintf(&temp_str, template, "", path, 1, (current == 1) ? "" : "in", 1);
	
	for(i = 2; i * *map_get(&vert_settings, "elem_per_page") <= maximum; i++)
	{
		log_trace("heyheyheyhey %d %d %d %d", i, i * *map_get(&vert_settings, "elem_per_page"), maximum, current);
		vert_util_asprintf(&temp_str, template, temp_str, path, i, (current == i) ? "" : "in", i);
	}
	 */
	
	return temp_str;
}

int vert_custom_delete_usericon(char *userid)
{
	char *filename = NULL, *final_file = NULL;
	int ret;
	
	
	filename = vert_custom_get_id_user_field(userid, "pfp");
	
	if(strncmp(filename, "/default_usericons", strlen("/default_usericons")) != 0)
	{
		log_info("deleting user %s's pfp", userid);
		
		if(filename)
		{
			vert_util_asprintf(&final_file, "html%s", filename);
			
			ret = remove(final_file);
		}
	}
	
	vert_util_safe_free(filename);
	vert_util_safe_free(final_file);
	
	return ret;
}

size_t vert_custom_get_entrycount_for_user(sb_Event *e)
{
	
}