#ifndef POST_H__
#define POST_H__

#include <time.h>
#include "user.h"

typedef struct
{
  char *title;
  char *image_loc;
  char *body;
  char *author_id;
  unsigned char is_published;
  char *unpublished_body;
  time_t created;
  time_t modified;
} post_t;

int vert_create_post(unsigned char type, char *title, char *icon, user_t *user);

int vert_get_post(char *title, post_t *post);

#endif
