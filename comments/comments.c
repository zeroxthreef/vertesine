#include "../util.h"
#include "../home/user.h"

#include "../lib/snowflake/snowflake.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <kcgi.h>
#include <curl/curl.h>
#include <sodium.h>
#include <pthread.h>
#include <time.h>
#include <jansson.h>

#include "comments.h"


struct kfcgi *fcgi;
short vert_shutdown = 0;




void respond_nothing(struct kreq *req)
{
  khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_404]);
  khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_HTML]);
  khttp_head(req, kresps[KRESP_CACHE_CONTROL], "no-cache");
  khttp_head(req, kresps[KRESP_EXPIRES], "Tue, 01 Jun 1999 19:58:02 GMT");
  khttp_body(req);
  khttp_puts(req, "<html><h1>404</h1></html>");
}

void respond_home(struct kreq *req)
{
  khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
  khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_HTML]);
  khttp_head(req, kresps[KRESP_CACHE_CONTROL], "no-cache");
  khttp_head(req, kresps[KRESP_EXPIRES], "Tue, 01 Jun 1999 19:58:02 GMT");
  khttp_body(req);
  khttp_puts(req, "<html>comments go here, yo</html>");
}

int respond_post(struct kreq *req)
{
  char *final_str = NULL;
  unqlite_kv_cursor *cur;
  unqlite_int64 v_size;
  char *key_value, *value_value;
  int k_size, can_followthrough = 0;





  if(req->method == KMETHOD_POST)
  {
    khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_HTML]);


    if(req->fieldsz == 1)
    {


      fprintf(stderr, "json: %s\n", req->fields[0].val);

      /*
      if(unqlite_kv_cursor_init(com_db, &cur) != UNQLITE_OK)
      {
        fprintf(stderr, "cant allocate a cursor\n");
        return 1;
      }

      /* search for the login key match */
      /*
      unqlite_kv_cursor_first_entry(cur);

      while(unqlite_kv_cursor_valid_entry(cur))
      {
        unqlite_kv_cursor_key(cur, NULL, &k_size);/* TODO get the size then allocate then get the key, and repeat for the value *//*
        key_value = malloc(k_size + 1);
        unqlite_kv_cursor_key(cur, key_value, &k_size);
        key_value[k_size] = 0x00;

        unqlite_kv_cursor_data(cur, NULL, &v_size);
        value_value = malloc(v_size + 1);
        unqlite_kv_cursor_data(cur, value_value, &v_size);
        value_value[v_size] = 0x00;

        k_size = 0;
        v_size = 0;



        if(strcmp(req->fields[0].val, key_value) == 0)
        {
          free(key_value);
          free(value_value);


          if(unqlite_kv_store(com_db, req->fields[0].val, -1, STRING HERE, strlen(STRING HERE) + 1) != UNQLITE_OK)
          {
            unqlite_rollback(com_db);
            fprintf(stderr, "couldn't add a value\n");

            return 1;
          }


        }
        else
        {
          unqlite_kv_cursor_next_entry(cur);


          free(value_value);
          free(key_value);
        }

      }
      */
      vert_asprintf(&final_str, "<html>oof</html>");
    }
    else
      vert_asprintf(&final_str, "<html>bad POST</html>");
  }
  else
  {
    khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_HTML]);


    vert_asprintf(&final_str, "<html>only for writing stuff</html>");

  }

  khttp_head(req, kresps[KRESP_CACHE_CONTROL], "no-cache");
  khttp_head(req, kresps[KRESP_EXPIRES], "Tue, 01 Jun 1999 19:58:02 GMT");

  khttp_body(req);
  khttp_puts(req, final_str);

  free(final_str);
}

void respond_get(struct kreq *req)
{
  char *final_str = NULL;
  json_t *response = json_object();;


  if(0)
  {
    khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_APP_JSON]);
  }
  else
  {
    khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_404]);
    khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_APP_JSON]);
    json_object_set(response, "status", json_string("NO_COMMENTS"));
  }

  khttp_head(req, kresps[KRESP_CACHE_CONTROL], "no-cache");
  khttp_head(req, kresps[KRESP_EXPIRES], "Tue, 01 Jun 1999 19:58:02 GMT");


  vert_asprintf(&final_str, json_dumps(response, 0));





  json_decref(response);
  khttp_body(req);
  khttp_puts(req, final_str);

  free(final_str);
}

short init()
{
  if(khttp_fcgi_init(&fcgi, NULL, 0, NULL, 0, 0) != KCGI_OK)
    return 1;


  if(unqlite_open(&com_db, CGI_ROOT "vdata/comments.db", UNQLITE_OPEN_CREATE) != UNQLITE_OK)
  {
    fprintf(stderr, "couldnt open an essential database\n");
    return 1;
  }

  if(sodium_init() < 0)
  {
    fprintf(stderr, "sodium init error\n");
    return 1;
  }

  if(snowflake_init(0, 1) == -1) /* worker id 0 for home, 1 for api, and count up from there */
  {
    fprintf(stderr, "snowflake init error\n");
    return 1;
  }

  curl_global_init(CURL_GLOBAL_DEFAULT);

  return 0;
}


void route()
{
  struct kreq req;

  while(khttp_fcgi_parse(fcgi, &req) == KCGI_OK && !vert_shutdown)
  {
    fprintf(stderr, "connection from %s requesting %s\n", req.remote, req.path); /* temporary to tell when it hangs */
    /* route requests */
    if(strcmp(req.path, "") == 0)
      respond_home(&req);
    else if(strncmp(req.path, "post_comment", strlen("post_comment")) == 0)
      respond_post(&req);
    else if(strncmp(req.path, "get_comment", strlen("get_comment")) == 0)
      respond_get(&req);
    else /* last resort if it doesn't find anything */
      respond_nothing(&req);
    /* ============= */

    khttp_free(&req);
  }

}

short destroy()
{
  khttp_fcgi_free(fcgi);
  unqlite_close(com_db);
  return 0;
}

int main(int argc, char **argv)
{
  if(init())
  {
    fprintf(stderr, "couldn't start\n");
    if(destroy())
      fprintf(stderr, "couldn't cleanly exit\n");
    else
      fprintf(stderr, "cleanly exitted\n");
  }

  route();

  if(destroy())
    fprintf(stderr, "couldn't cleanly exit\n");
  else
    fprintf(stderr, "cleanly exitted\n");
  return 0;
}
