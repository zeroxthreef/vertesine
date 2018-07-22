#ifndef USER_H__
#define USER_H__

#include <sys/types.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <kcgi.h>
#include <time.h>

typedef struct
{
  char *username;
  char *email;
  char *pw_hash;
  unsigned int permissions;
  time_t jointime;
  char *verifyurl;
  unsigned int is_verified;
  unsigned int is_deleted;
  unsigned int birthday_public;
  char *reg_ip;
  char *id;

  char *bio;
  char *twitter;
  char *youtube_ch;
  char *website;
  time_t birthday;
  char *avatar;
} user_t;


int vert_create_user(char *username, char *email, char *password_hash, char *register_ip);

int vert_verify_user(char *verify_key);

int vert_login_user(char *username, char *pass_hash, char **token);

int vert_logout_user();

int vert_edit_user(char *username, char *user_id, user_t *set_user);

int vert_view_user(user_t **user_data, char *user_id, char *username);

int vert_is_user_logged_in(struct kreq *req, user_t **user_data);

void vert_clean_user(user_t *user);

#endif
