#include "post.h"
#include "user.h"
#include "../util.h"

#include "../lib/snowflake/snowflake.h"

#include <unqlite.h>
#include <jansson.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int vert_create_post(unsigned char type, char *title, char *icon, user_t *user)
{
  unqlite_kv_cursor *cur;
  char *temp_path = NULL, *key_value, *value_value;
  unqlite_int64 v_size, user_index = 0;
  int k_size;
  json_t *post = NULL;


  if(unqlite_kv_cursor_init(post_db, &cur) != UNQLITE_OK)
  {
    fprintf(stderr, "cant allocate a cursor\n");
    return 1;
  }

  unqlite_kv_cursor_first_entry(cur);

  while(unqlite_kv_cursor_valid_entry(cur))
  {
    unqlite_kv_cursor_key(cur, NULL, &k_size);/* TODO get the size then allocate then get the key, and repeat for the value */
    key_value = calloc(1, k_size + 1);
    unqlite_kv_cursor_key(cur, key_value, &k_size);
    key_value[k_size] = 0x00;

    unqlite_kv_cursor_data(cur, NULL, &v_size);
    value_value = calloc(1, v_size + 1);
    unqlite_kv_cursor_data(cur, value_value, &v_size);
    value_value[v_size] = 0x00;

    vert_filter_jansson_breakers(value_value, v_size);

    k_size = 0;
    v_size = 0;


    if(strcmp(key_value, title) == 0)
    {
      unqlite_kv_cursor_release(post_db, cur);
      free(key_value);
      free(value_value);
      return 2;
    }

    unqlite_kv_cursor_next_entry(cur);




    free(key_value);
    free(value_value);
  }

  fprintf(stderr, "creating post %s\n", title);

  post = json_object();

  json_object_set_new(post, "title", json_string(title));
  json_object_set_new(post, "image_loc", json_string(""));
  json_object_set_new(post, "body", json_string(""));
  json_object_set_new(post, "author_id", json_string(user->id));
  json_object_set_new(post, "unpublished_body", json_string(""));
  json_object_set_new(post, "created", json_integer(time(NULL)));
  json_object_set_new(post, "edited", json_integer(time(NULL)));


  if(unqlite_kv_store(post_db, title, -1, json_dumps(post, 0), strlen(json_dumps(post, 0)) + 1) != UNQLITE_OK && json_dumps(post, 0) == NULL)
  {
    unqlite_rollback(post_db);
    unqlite_kv_cursor_release(post_db, cur);
    fprintf(stderr, "couldn't add a value\n");
    json_decref(post);

    return 3;
  }

  vert_asprintf(&temp_path, "entries/%s", title);

  if(vert_create_html_dir_with_cgi_index(temp_path))
    fprintf(stderr, "entry creation unfinished for [%s]\n");

  free(temp_path);
  unqlite_kv_cursor_release(post_db, cur);
  json_decref(post);
  unqlite_commit(post_db);

  return 0;
}

int vert_get_post(char *title, post_t *post)
{

  return 0;
}

int vert_edit_post(post_t *post)
{


  return 0;
}
