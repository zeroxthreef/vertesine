#ifndef VERTESINE_UTIL_H__
#define VERTESINE_UTIL_H__

#include "../util.h"

#include <hiredis/hiredis.h>

#include <time.h>

#define VERT_USER_VERSION 2

typedef struct
{
	char *username;
	char *displayname;
	char *email;
	char *pw_hash;
	char *verifyurl;
	char *token;
	time_t jointime;
	unsigned int is_verified;
	unsigned int permissions;
	unsigned int birthday_public;
	char *register_ip;
	char *id;
	
	char *bio;
	char *twitter;
	char *youtube;
	char *website;
	time_t birthday;
	char *avatar;
} vert_custom_user_t;

enum vert_custom_types
{
	VERT_CUSTOM_OBJECT_USER,
	VERT_CUSTOM_OBJECT_ENTRY,
	VERT_CUSTOM_OBJECT_PROJECT,
	VERT_CUSTOM_OBJECT_COMMENT,
	VERT_CUSTOM_OBJECT_UTOKEN,
	VERT_CUSTOM_OBJECT_MISC
};


redisContext *vert_redis_ctx;


int vert_custom_init();

void vert_custom_destroy();

char *vert_custom_generate_db_key(uint64_t id, int type);

uint64_t vert_custom_db_get_id(char *db_key);

int vert_custom_db_get_type(char *db_key);

char *vert_custom_random_string(unsigned long maxlen);

void vert_custom_set_get_cookie(sb_Event *e, char *cookie_str, int send);

char *vert_custom_get_http_date(time_t time_num);

uint64_t vert_custom_logged_in(sb_Event *e);

char *vert_custom_get_id_user_field(char *id, char *field);

char *vert_custom_get_username_user_field(char *username, char *field);

int vert_custom_check_user_exists(char *username);

uint32_t vert_custom_get_username_user_permissions(char *username);

void vert_custom_filter_input(char **buffer, uint8_t depth);

void vert_custom_filter_tags(char **buffer);

uint8_t vert_custom_write_hash_field(uint64_t id, int object_type, char *field, char *data);

int vert_custom_manage_pfp(uint8_t *data, char *extension, char *user_id, unsigned long size, char **pfp_loc);

char *vert_custom_determine_extension(uint8_t *data, size_t data_len);

char *vert_custom_generate_pagenum_nav(char *path, size_t current, size_t maximum);

int vert_custom_delete_usericon(char *userid);

char *vert_custom_get_id_generic_field(char *id, int type, char *field); /* only for hash keys */

char *vert_custom_get_entryname_field(char *entry_title, char *field);

int vert_contains_perm(uint32_t permissions, int one_indexed_bit);

char *vert_extract_list_elem(char *list_elem, int index);

size_t vert_get_entry_count(sb_Event *e);

int vert_can_view_entry(char *id_entry, sb_Event *e);

#endif