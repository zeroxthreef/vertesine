#ifndef UTIL_H__
#define UTIL_H__

#include <unqlite.h>

#define CGI_ROOT "/var/www/cgi_data/vertesine/"
#define HTML_ROOT "/var/www/html/vertesine/"

/* global vars */
/*
enum cookie
{
  COOKIE_STRING,
  COOKIE_INTEGER,
  COOKIE__MAX
};

const struct kvalid cookies[COOKIE__MAX] = {
  {
    kvalid_stringne, "string"
  },
  {
    kvalid_int, "integer"
  }
};
*/

unqlite *acc_db, *post_db, *com_db;

/* functions */

short vert_asprintf(char **string, const char *fmt, ...);

unsigned char *vert_read_file(const char *location, unsigned long *size_ptr);

int vert_add_inotify_watch(int *fd, int *watch_dir, char *dir);

void vert_check_change(int *fd, void(*vert_ch_callback)(char *path));

void vert_clean_inotify(int *fd, int *watch_dir);

char *vert_generate_random_string(unsigned long maxlen);

int vert_manage_pfp(unsigned char *data, char *extension, char *user_id, unsigned long size, char **pfp_loc);

void vert_filter_jansson_breakers(char *string, unsigned long terminated_len);

void vert_filter_html(char *string, unsigned long terminated_len);

int vert_contains_html(char *string);

int vert_parse_markdown(char *string, char **output);

char *vert_create_page_list(unsigned int current, unsigned int max, char *query_root);

unsigned long vert_get_db_keyv_num(void *db);

int vert_create_html_dir_with_cgi_index(char *path);

char *vert_replace(char *original, char *find, char *replace);

char *vert_replace_all(char *original, char *find, char *replace);

int vert_replace_buffer_all(char **original, char *find, char *replace);

char *vert_strdup(char *original);

#endif
