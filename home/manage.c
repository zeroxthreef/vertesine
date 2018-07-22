#include "manage.h"
#include "user.h"
#include "../util.h"

#include "../lib/snowflake/snowflake.h"

#include <sys/types.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <kcgi.h>
#include <unqlite.h>
#include <sodium.h>
#include <jansson.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int vert_generate_manage_html(char **final_html)
{
  unqlite_kv_cursor *cur;
  unqlite_int64 v_size;
  char *key_value, *value_value, *login_key = NULL, *table_final = NULL, *table_part = NULL;
  int k_size, can_followthrough = 0;
  user_t *usr_ptr = NULL;
    /* test if the user id is null, then use the username */


    if(unqlite_kv_cursor_init(acc_db, &cur) != UNQLITE_OK)
    {
      fprintf(stderr, "cant allocate a cursor\n");
      return 1;
    }

    /* search for the login key match */

    vert_asprintf(&table_final, "<tr><th>ID</th><th>value</th></tr>\n");

    unqlite_kv_cursor_first_entry(cur);

    while(unqlite_kv_cursor_valid_entry(cur))
    {
      unqlite_kv_cursor_key(cur, NULL, &k_size);/* TODO get the size then allocate then get the key, and repeat for the value */
      key_value = malloc(k_size + 1);
      unqlite_kv_cursor_key(cur, key_value, &k_size);
      key_value[k_size] = 0x00;

      unqlite_kv_cursor_data(cur, NULL, &v_size);
      value_value = malloc(v_size + 1);
      unqlite_kv_cursor_data(cur, value_value, &v_size);
      value_value[v_size] = 0x00;

      vert_filter_html(value_value, v_size);

      k_size = 0;
      v_size = 0;




      vert_asprintf(&table_final, "%s<tr><td>%s</td><td style=\"word-break: break-all;\">%s</td></tr>\n", table_final, key_value, value_value);


      unqlite_kv_cursor_next_entry(cur);


      free(value_value);
      free(key_value);
    }

    vert_asprintf(final_html, "<table style=\"width:100%%;\">%s</table><br><br>Edit User ID:<br>", table_final);


    free(table_final);

  return 0;
}

int vert_post_manage_html(char *id, char *value)
{
  unqlite_kv_cursor *cur;
  unqlite_int64 v_size;
  char *key_value, *value_value, *login_key = NULL;
  int k_size, can_followthrough = 0;
  /* test if the user id is null, then use the username */


  if(unqlite_kv_cursor_init(acc_db, &cur) != UNQLITE_OK)
  {
    fprintf(stderr, "cant allocate a cursor\n");
    return 1;
  }

  /* search for the login key match */

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

    vert_filter_jansson_breakers(&value_value, v_size);

    k_size = 0;
    v_size = 0;


    json_t *user = json_loads(value, 0, NULL);
    if(user == NULL && strcmp(value, "!delete") != 0)
    {
      free(key_value);
      free(value_value);
      fprintf(stderr, "jansson gave up\n");
      return 1; /* db error */
    }

    /* check if the key matches and make the user */



    if(strcmp(id, key_value) == 0)
    {


      if(strcmp(value, "!delete") == 0)
      {
        if(unqlite_kv_delete(acc_db, key_value, -1) != UNQLITE_OK)
        {
          unqlite_rollback(acc_db);
          free(key_value);
          free(value_value);
          fprintf(stderr, "couldn't remove user\n");
          json_decref(user);

          return 1;
        }
        else
          fprintf(stderr, "removing user %s\n", key_value);
      }
      else if(unqlite_kv_store(acc_db, id, -1, value, strlen(value) + 1) != UNQLITE_OK)
      {
        unqlite_rollback(acc_db);
        free(key_value);
        free(value_value);
        fprintf(stderr, "couldn't add a value\n");
        json_decref(user);

        return 1;
      }





      fprintf(stderr, "managing user %s\n", id);
      unqlite_commit(acc_db);
      free(key_value);
      free(value_value);
      json_decref(user);
      return 2;
    }

    unqlite_kv_cursor_next_entry(cur);


    free(value_value);
    free(key_value);
    json_decref(user);
  }

  return 0;
}

int vert_manage_get_value(char *id, char **value)
{
  unqlite_kv_cursor *cur;
  unqlite_int64 v_size;
  char *key_value, *value_value, *login_key = NULL;
  int k_size, can_followthrough = 0;
  user_t *usr_ptr = NULL;
    /* test if the user id is null, then use the username */


    if(unqlite_kv_cursor_init(acc_db, &cur) != UNQLITE_OK)
    {
      fprintf(stderr, "cant allocate a cursor\n");
      return 1;
    }

    /* search for the login key match */

    unqlite_kv_cursor_first_entry(cur);

    while(unqlite_kv_cursor_valid_entry(cur))
    {
      unqlite_kv_cursor_key(cur, NULL, &k_size);/* TODO get the size then allocate then get the key, and repeat for the value */
      key_value = malloc(k_size + 1);
      unqlite_kv_cursor_key(cur, key_value, &k_size);
      key_value[k_size] = 0x00;

      unqlite_kv_cursor_data(cur, NULL, &v_size);
      value_value = malloc(v_size + 1);
      unqlite_kv_cursor_data(cur, value_value, &v_size);
      value_value[v_size] = 0x00;

      k_size = 0;
      v_size = 0;



      if(strcmp(id, key_value) == 0)
      {
        free(key_value);
        *value = value_value;
        return 2;
      }


      unqlite_kv_cursor_next_entry(cur);


      free(value_value);
      free(key_value);
    }


  return 0;
}
