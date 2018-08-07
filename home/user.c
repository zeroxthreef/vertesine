#include "user.h"
#include "../util.h"

#include "../lib/snowflake/snowflake.h"

#include <unqlite.h>
#include <sodium.h>
#include <jansson.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

size_t pl;


int vert_create_user(char *username, char *email, char *password_hash, char *register_ip)
{
  unqlite_kv_cursor *cur;
  char *temp_snowflake = NULL, *key_value, *value_value, *temp_email, *temp_username, *temp_randstr;
  unqlite_int64 v_size, user_index = 0;
  int k_size;


  if(unqlite_kv_cursor_init(acc_db, &cur) != UNQLITE_OK)
  {
    fprintf(stderr, "cant allocate a cursor\n");
    return 1;
  }
  /*
  if(unqlite_kv_fetch(acc_db, username, -1, NULL, &pl) == UNQLITE_OK)
  {
    fprintf(stderr, "user already exists\n");
    return 1;
  }
  */
  vert_asprintf(&temp_snowflake, "%ld", snowflake_id());

  /* search for username or email matches */

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

    /* create temporary json object */

    json_t *user = json_loads(value_value, 0, NULL);
    if(user == NULL)
    {
      free(key_value);
      free(value_value);
      unqlite_kv_cursor_release(acc_db, cur);
      fprintf(stderr, "jansson gave up\n");
      return 1;
    }



    temp_email = json_string_value(json_object_get(user, "email"));
    temp_username = json_string_value(json_object_get(user, "username"));

    if(temp_email == NULL || temp_username == NULL)
    {
      free(key_value);
      free(value_value);
      unqlite_kv_cursor_release(acc_db, cur);
      json_decref(user);
      fprintf(stderr, "jansson couldnt find the strings. Pretty bad error\n");
      return 1;
    }

    /* fprintf(stderr, "key: %s  value: %s ||| email %s  username: %s\n", key_value, value_value, temp_email, temp_username); */

    if(strcmp(temp_email, email) == 0)
    {
      //free(temp_email);
      //free(temp_username);
      unqlite_kv_cursor_release(acc_db, cur);
      json_decref(user);
      free(key_value);
      free(value_value);
      return 3;
    }

    if(strcmp(temp_username, username) == 0)
    {
      //free(temp_email);
      //free(temp_username);
      unqlite_kv_cursor_release(acc_db, cur);
      json_decref(user);
      free(key_value);
      free(value_value);
      return 2;
    }


    unqlite_kv_cursor_next_entry(cur);

    //free(temp_email);
    //free(temp_username);
    user_index++;
    json_decref(user);
    free(key_value);
    free(value_value);
  }

  unqlite_kv_cursor_release(acc_db, cur);

  /* add the user to the db */

  temp_randstr = vert_generate_random_string(50); /* takes a long time anyway so whatever */


  json_t *user = json_object();

  json_object_set_new(user, "username", json_string(username));
  json_object_set_new(user, "email", json_string(email));
  json_object_set_new(user, "pass_hash", json_string(password_hash));
  if(user_index == 0)
    json_object_set_new(user, "permissions", json_integer(3)); /* 0 is regular user 1 is moderator 2 is admin and 3 is just do whatever i guess permission */
  else
    json_object_set_new(user, "permissions", json_integer(0)); /* 0 is regular user 1 is moderator 2 is admin and 3 is just do whatever i guess permission */
  json_object_set_new(user, "join_time", json_integer(time(NULL)));
  json_object_set_new(user, "verify_url", json_string(temp_randstr));
  json_object_set_new(user, "verified", json_integer(0));
  json_object_set_new(user, "deleted", json_integer(0));
  json_object_set_new(user, "birthday_viewable", json_integer(0));
  json_object_set_new(user, "register_ip", json_string(register_ip)); /* for tracing spam accounts (ive learned my lesson. No more stupid anonymous posting. Some people really suck) */


  json_object_set_new(user, "bio", json_string(""));
  json_object_set_new(user, "twitter", json_string(""));
  json_object_set_new(user, "youtube_channel", json_string(""));
  json_object_set_new(user, "website", json_string(""));
  json_object_set_new(user, "birthday", json_integer(0));

  switch(rand() % 4)
  {
    case 0:
      json_object_set_new(user, "avatar", json_string("/default_usericons/usericon.png"));
    break;
    case 1:
      json_object_set_new(user, "avatar", json_string("/default_usericons/usericon1.png"));
    break;
    case 2:
      json_object_set_new(user, "avatar", json_string("/default_usericons/usericon2.png"));
    break;
    case 3:
      json_object_set_new(user, "avatar", json_string("/default_usericons/usericon3.png"));
    break;
  }


  if(unqlite_kv_store(acc_db, temp_snowflake, -1, json_dumps(user, 0), strlen(json_dumps(user, 0)) + 1) != UNQLITE_OK && json_dumps(user, 0) == NULL)
  {
    unqlite_rollback(acc_db);
    free(temp_randstr);
    free(temp_snowflake);
    fprintf(stderr, "couldn't add a value\n");
    json_decref(user);

    return 1;
  }





  fprintf(stderr, "adding user %s index %lu\n", username, user_index);
  unqlite_commit(acc_db);
  free(temp_randstr);
  free(temp_snowflake);
  json_decref(user);

  return 0;
}

int vert_verify_user(char *verify_key)
{

  return 0;
}

int vert_login_user(char *username, char *pass_hash, char **token)
{
  unqlite_kv_cursor *cur;
  unqlite_int64 v_size;
  char *key_value, *value_value;
  int k_size;


  if(unqlite_kv_cursor_init(acc_db, &cur) != UNQLITE_OK)
  {
    fprintf(stderr, "cant allocate a cursor\n");
    return 1;
  }

  /* search for username or email matches */

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



    json_t *user = json_loads(value_value, 0, NULL);
    if(user == NULL)
    {
      free(key_value);
      free(value_value);
      unqlite_kv_cursor_release(acc_db, cur);
      fprintf(stderr, "jansson gave up\n");
      return 1; /* db error */
    }

    if(strcmp(json_string_value(json_object_get(user, "username")), username) == 0)
    {
      if(crypto_pwhash_str_verify(json_string_value(json_object_get(user, "pass_hash")), pass_hash, strlen(pass_hash)) != 0)
      {
        free(value_value);
        free(key_value);
        unqlite_kv_cursor_release(acc_db, cur);
        json_decref(user);

        return 3; /* login unsuccessful */
      }


      vert_asprintf(token, "%sU%s", key_value, json_string_value(json_object_get(user, "verify_url")));

      free(value_value);
      free(key_value);
      json_decref(user);
      unqlite_kv_cursor_release(acc_db, cur);

      return 2; /* login successful */
    }

    unqlite_kv_cursor_next_entry(cur);

    free(value_value);
    free(key_value);
    json_decref(user);
  }

  unqlite_kv_cursor_release(acc_db, cur);


  return 0; /* couldnt find user */
}

int vert_logout_user()
{

  return 0;
}

int vert_edit_user(char *username, char *user_id, user_t *set_user)
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


    json_t *user = json_loads(value_value, 0, NULL);
    if(user == NULL)
    {
      free(key_value);
      free(value_value);
      unqlite_kv_cursor_release(acc_db, cur);
      fprintf(stderr, "jansson gave up\n");
      return 1; /* db error */
    }


    /* fprintf(stderr, "ttt: %s %s %s\n", req->cookies[0].key, req->cookies[0].val, login_key); */

    /* check if the key matches and make the user */
    if(username != NULL)
    {
      if(strcmp(username, json_string_value(json_object_get(user, "username"))) == 0)
      {
        can_followthrough = 1;
      }
    }

    if(user_id != NULL)
    {
      if(strcmp(user_id, key_value) == 0)
      {
        can_followthrough = 1;
      }
    }


    if(can_followthrough)
    {



      /* json_t *user = json_object(); */

      json_object_set(user, "username", json_string(set_user->username));
      json_object_set(user, "email", json_string(set_user->email));
      json_object_set(user, "pass_hash", json_string(set_user->pw_hash));
      json_object_set(user, "permissions", json_integer(set_user->permissions)); /* 0 is regular user 1 is moderator 2 is admin and 3 is just do whatever i guess permission */
      json_object_set(user, "join_time", json_integer(set_user->jointime));
      json_object_set(user, "verify_url", json_string(set_user->verifyurl));
      json_object_set(user, "verified", json_integer(set_user->is_verified));
      json_object_set(user, "deleted", json_integer(set_user->is_deleted));
      json_object_set(user, "birthday_viewable", json_integer(set_user->birthday_public));
      json_object_set(user, "register_ip", json_string(set_user->reg_ip)); /* for tracing spam accounts (ive learned my lesson. No more stupid anonymous posting. Some people really suck) */


      json_object_set(user, "bio", json_string(set_user->bio));
      json_object_set(user, "twitter", json_string(set_user->twitter));
      json_object_set(user, "youtube_channel", json_string(set_user->youtube_ch));
      json_object_set(user, "website", json_string(set_user->website));
      json_object_set(user, "birthday", json_integer(set_user->birthday));
      json_object_set(user, "avatar", json_string(set_user->avatar));




      if(unqlite_kv_store(acc_db, set_user->id, -1, json_dumps(user, 0), strlen(json_dumps(user, 0)) + 1) != UNQLITE_OK && json_dumps(user, 0) == NULL)
      {
        unqlite_rollback(acc_db);
        free(key_value);
        free(value_value);
        unqlite_kv_cursor_release(acc_db, cur);
        fprintf(stderr, "couldn't add a value\n");
        json_decref(user);

        return 1;
      }





      fprintf(stderr, "updating user %s\n", username);
      unqlite_commit(acc_db);
      unqlite_kv_cursor_release(acc_db, cur);
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

  unqlite_kv_cursor_release(acc_db, cur);

  return 0;
}

int vert_view_user(user_t **user_data, char *user_id, char *username)
{
  unqlite_kv_cursor *cur;
  unqlite_int64 v_size;
  char *key_value, *value_value, *login_key = NULL;
  int k_size, can_followthrough = 0;
  user_t *usr_ptr = NULL;
  json_error_t jerror;
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

      vert_filter_jansson_breakers(value_value, v_size);

      k_size = 0;
      v_size = 0;


      json_t *user = json_loads(value_value, 0, &jerror);
      if(user == NULL)
      {
        free(key_value);
        free(value_value);
        unqlite_kv_cursor_release(acc_db, cur);
        fprintf(stderr, "jansson gave up %s %s %d %d\n", jerror.text, jerror.source, jerror.line, jerror.column);
        return 1; /* db error */
      }


      /* fprintf(stderr, "ttt: %s %s %s\n", req->cookies[0].key, req->cookies[0].val, login_key); */

      /* check if the key matches and make the user */
      if(username != NULL)
      {
        if(strcmp(username, json_string_value(json_object_get(user, "username"))) == 0)
        {
          can_followthrough = 1;
        }
      }

      if(user_id != NULL)
      {
        if(strcmp(user_id, key_value) == 0)
        {
          can_followthrough = 1;
        }
      }


      if(can_followthrough)
      {
        free(value_value);



        if((*user_data = calloc(1, sizeof(user_t))) == NULL)
        {
          fprintf(stderr, "allocation error\n");
          free(key_value);
          json_decref(user);
          return 1;
        }

        usr_ptr = *user_data;

        usr_ptr->username = vert_strdup(json_string_value(json_object_get(user, "username")));
        usr_ptr->email = vert_strdup(json_string_value(json_object_get(user, "email")));
        usr_ptr->pw_hash = vert_strdup(json_string_value(json_object_get(user, "pass_hash")));
        usr_ptr->permissions = json_integer_value(json_object_get(user, "permissions"));
        usr_ptr->jointime = json_integer_value(json_object_get(user, "join_time"));
        usr_ptr->verifyurl = vert_strdup(json_string_value(json_object_get(user, "verify_url")));
        usr_ptr->is_verified = json_integer_value(json_object_get(user, "verified"));
        usr_ptr->is_deleted = json_integer_value(json_object_get(user, "deleted"));
        usr_ptr->birthday_public = json_integer_value(json_object_get(user, "birthday_viewable"));
        usr_ptr->reg_ip = vert_strdup(json_string_value(json_object_get(user, "register_ip")));
        usr_ptr->id = vert_strdup(key_value);

        usr_ptr->bio = vert_strdup(json_string_value(json_object_get(user, "bio")));
        usr_ptr->twitter = vert_strdup(json_string_value(json_object_get(user, "twitter")));
        usr_ptr->youtube_ch = vert_strdup(json_string_value(json_object_get(user, "youtube_channel")));
        usr_ptr->website = vert_strdup(json_string_value(json_object_get(user, "website")));
        usr_ptr->birthday = json_integer_value(json_object_get(user, "birthday"));
        usr_ptr->avatar = vert_strdup(json_string_value(json_object_get(user, "avatar")));


        free(key_value);
        json_decref(user);
        unqlite_kv_cursor_release(acc_db, cur);
        return 2;
      }

      unqlite_kv_cursor_next_entry(cur);


      free(value_value);
      free(key_value);
      json_decref(user);
    }

    unqlite_kv_cursor_release(acc_db, cur);

  return 0;
}

int vert_is_user_logged_in(struct kreq *req, user_t **user_data)
{
  unqlite_kv_cursor *cur;
  unqlite_int64 v_size;
  char *key_value, *value_value, *login_key = NULL;
  int k_size;
  user_t *usr_ptr = NULL;

  if(req->cookiesz == 0)
    return 3;


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

    vert_filter_jansson_breakers(value_value, v_size);

    k_size = 0;
    v_size = 0;

    json_t *user = json_loads(value_value, 0, NULL);
    if(user == NULL)
    {
      free(key_value);
      free(value_value);
      unqlite_kv_cursor_release(acc_db, cur);
      fprintf(stderr, "jansson gave up\n");
      return 1; /* db error */
    }

    vert_asprintf(&login_key, "%sU%s", key_value, json_string_value(json_object_get(user, "verify_url")));

    /* fprintf(stderr, "ttt: %s %s %s\n", req->cookies[0].key, req->cookies[0].val, login_key); */

    /* check if the key matches and make the user */

    if(strcmp(login_key, req->cookies[0].val) == 0)
    {
      free(login_key);
      free(value_value);



      if((*user_data = calloc(1, sizeof(user_t))) == NULL)
      {
        fprintf(stderr, "allocation error\n");
        free(key_value);
        json_decref(user);
        unqlite_kv_cursor_release(acc_db, cur);
        return 1;
      }

      usr_ptr = *user_data;

      usr_ptr->username = vert_strdup(json_string_value(json_object_get(user, "username")));
      usr_ptr->email = vert_strdup(json_string_value(json_object_get(user, "email")));
      usr_ptr->pw_hash = vert_strdup(json_string_value(json_object_get(user, "pass_hash")));
      usr_ptr->permissions = json_integer_value(json_object_get(user, "permissions"));
      usr_ptr->jointime = json_integer_value(json_object_get(user, "join_time"));
      usr_ptr->verifyurl = vert_strdup(json_string_value(json_object_get(user, "verify_url")));
      usr_ptr->is_verified = json_integer_value(json_object_get(user, "verified"));
      usr_ptr->is_deleted = json_integer_value(json_object_get(user, "deleted"));
      usr_ptr->birthday_public = json_integer_value(json_object_get(user, "birthday_viewable"));
      usr_ptr->reg_ip = vert_strdup(json_string_value(json_object_get(user, "register_ip")));
      usr_ptr->id = vert_strdup(key_value);

      usr_ptr->bio = vert_strdup(json_string_value(json_object_get(user, "bio")));
      usr_ptr->twitter = vert_strdup(json_string_value(json_object_get(user, "twitter")));
      usr_ptr->youtube_ch = vert_strdup(json_string_value(json_object_get(user, "youtube_channel")));
      usr_ptr->website = vert_strdup(json_string_value(json_object_get(user, "website")));
      usr_ptr->birthday = json_integer_value(json_object_get(user, "birthday"));
      usr_ptr->avatar = vert_strdup(json_string_value(json_object_get(user, "avatar")));

      free(key_value);
      json_decref(user);
      return 2;
    }

    unqlite_kv_cursor_next_entry(cur);


    free(login_key);
    login_key = NULL;
    free(value_value);
    free(key_value);
    json_decref(user);
  }


  unqlite_kv_cursor_release(acc_db, cur);
  //free(value_value);
  //free(key_value);
  //if(req->cookies[0].key)
  return 0;
}

void vert_clean_user(user_t *user)
{
  if(user != NULL)
  {
    free(user->username);
    free(user->email);
    free(user->pw_hash);
    free(user->verifyurl);
    free(user->reg_ip);
    free(user->bio);
    free(user->twitter);
    free(user->youtube_ch);
    free(user->website);
    free(user->avatar);
    free(user->id);

    free(user);
  }
}
