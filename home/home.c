/* #include "home.h" */
#include "../util.h"
#include "user.h"

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

#include "home.h"

#define FOLDER_COUNT 9


struct kfcgi *fcgi;
short vert_shutdown = 0;
pthread_t file_update_thread;
const char *manage_key = NULL;

/* cached files */
char *home_html, *navbar_html, *notification_html, *notification_text, *error_template, *login_html, *register_html, *about_html, *login_html, *register_html, *confirm_logout_html, *user_html, *edit_user, *manage_html;
unsigned long home_html_len, navbar_html_len, notification_html_len, notification_text_len, error_template_len, login_html_len, register_html_len, about_html_len, login_html_len, register_html_len, confirm_logout_html_len, user_html_len, edit_user_len, manage_html_len;

int fd[FOLDER_COUNT], fwd[FOLDER_COUNT];

char *recent_mod = NULL;

/* functions */

short init()
{
  unsigned long i;


  if(khttp_fcgi_init(&fcgi, NULL, 0, NULL, 0, 0) != KCGI_OK)
    return 1;


  if((manage_key = getenv("VERTESINE_MANAGE_KEY")) == NULL)
  {
    fprintf(stderr, "Please set the VERTESINE_MANAGE_KEY variable\n");
    return 1;
  }

  if(vert_add_inotify_watch(&fd[0], &fwd[0], CGI_ROOT "."))
    return 1;
  if(vert_add_inotify_watch(&fd[1], &fwd[1], CGI_ROOT "about/."))
    return 1;
  if(vert_add_inotify_watch(&fd[2], &fwd[2], CGI_ROOT "entries/."))
    return 1;
  if(vert_add_inotify_watch(&fd[3], &fwd[3], CGI_ROOT "error/."))
    return 1;
  if(vert_add_inotify_watch(&fd[4], &fwd[4], CGI_ROOT "login/."))
    return 1;
  if(vert_add_inotify_watch(&fd[5], &fwd[5], CGI_ROOT "register/."))
    return 1;
  if(vert_add_inotify_watch(&fd[6], &fwd[6], CGI_ROOT "projects/."))
    return 1;
  if(vert_add_inotify_watch(&fd[6], &fwd[6], CGI_ROOT "confirm_logout/."))
    return 1;
  if(vert_add_inotify_watch(&fd[4], &fwd[4], CGI_ROOT "user/."))
    return 1;

  /* cache the files */
  home_html = vert_read_file(CGI_ROOT "home.html", &home_html_len);
  navbar_html = vert_read_file(CGI_ROOT "navbar.html", &navbar_html_len);
  notification_html = vert_read_file(CGI_ROOT "notification.html", &notification_html_len);
  notification_text = vert_read_file(CGI_ROOT "notifications.txt", &notification_text_len);
  about_html = vert_read_file(CGI_ROOT "about/about.html", &about_html_len);
  login_html = vert_read_file(CGI_ROOT "login/login.html", &login_html_len);
  register_html = vert_read_file(CGI_ROOT "register/register.html", &register_html_len);
  confirm_logout_html = vert_read_file(CGI_ROOT "confirm_logout/confirm_logout.html", &confirm_logout_html_len);
  user_html = vert_read_file(CGI_ROOT "user/user.html", &user_html_len);
  edit_user = vert_read_file(CGI_ROOT "user/edit_user.html", &edit_user_len);
  manage_html = vert_read_file(CGI_ROOT "manage.html", &manage_html_len);


  for(i = 0; i < FOLDER_COUNT; i++)
  {
    fprintf(stderr, "adding inotify watch to folder %lu out of %lu\n", i, FOLDER_COUNT);
    pthread_create(&file_update_thread, NULL, u_thread, &i);
    usleep(10*1000);
  }

  if(unqlite_open(&acc_db, CGI_ROOT "vdata/accounts.db", UNQLITE_OPEN_CREATE | UNQLITE_OPEN_READWRITE | UNQLITE_ACCESS_READWRITE) != UNQLITE_OK || unqlite_open(&post_db, CGI_ROOT "vdata/content.db", UNQLITE_OPEN_CREATE | UNQLITE_OPEN_READWRITE) != UNQLITE_OK)
  {
    fprintf(stderr, "couldnt open an essential database\n");
    return 1;
  }

  unqlite_commit(acc_db);

  if(sodium_init() < 0)
  {
    fprintf(stderr, "sodium init error\n");
    return 1;
  }

  if(snowflake_init(0, 0) == -1) /* worker id 0 for home, 1 for api, and count up from there */
  {
    fprintf(stderr, "snowflake init error\n");
    return 1;
  }

  curl_global_init(CURL_GLOBAL_DEFAULT);

  return 0;
}

void respond_debug(struct kreq *req)
{
  char *debug = NULL;

  vert_asprintf(&debug, "<h1>hello, hows it going</h1><p>debug values: [%s] [%s] [%s] [%s] [%s] [%s] [%s]<b><h2>recent file modification</h2>: %s<br><br>header: [error]", req->path, req->fullpath, req->host, req->pagename, req->pname, req->remote, req->suffix, recent_mod);

  khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
  khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_HTML]);
  khttp_head(req, kresps[KRESP_CACHE_CONTROL], "no-cache");
  khttp_head(req, kresps[KRESP_EXPIRES], "Tue, 01 Jun 1999 19:58:02 GMT");
  khttp_body(req);
  khttp_puts(req, debug);

  free(debug);
}

void respond_confirmlogout(struct kreq *req)
{
  user_t *user = NULL;
  char epoch_str[64];


  khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_302]);
  khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_HTML]);
  khttp_head(req, kresps[KRESP_CACHE_CONTROL], "no-cache");
  khttp_head(req, kresps[KRESP_EXPIRES], "Tue, 01 Jun 1999 19:58:02 GMT");
  khttp_head(req, kresps[KRESP_LOCATION], "/");

  if(vert_is_user_logged_in(req, &user) == 2)
  {
    khttp_head(req, kresps[KRESP_SET_COOKIE], "login_token=%s; Expires=%s; Secure; HttpOnly; Path=/", req->cookies[0].val, kutil_epoch2str(time(NULL) - (60 * 60 * 24), epoch_str, sizeof(epoch_str)));
  }


  khttp_body(req);

  khttp_puts(req, "attempting to log out");
  vert_clean_user(user);
}

void respond_home(struct kreq *req)
{
  char *return_page = NULL, *notification_text_return = NULL, *final_navbar = NULL, *final_userhtml = NULL;
  time_t time_date_time = time(NULL);
  struct tm time_date = *localtime(&time_date_time);
  user_t *user = NULL;

  khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
  khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_HTML]);
  khttp_head(req, kresps[KRESP_CACHE_CONTROL], "no-cache");
  khttp_head(req, kresps[KRESP_EXPIRES], "Tue, 01 Jun 1999 19:58:02 GMT");
  khttp_body(req);


  if(vert_is_user_logged_in(req, &user) == 2)
  {

    vert_asprintf(&final_userhtml, "<li class=\"navbarli\" style=\"float: right;\"><a class=\"navbarli\" href=\"/user/?u=%s\"><div style=\"usernavbar\"><img src=\"%s\" alt=\"usericon\" style=\"width:14px;height:14px;margin:0px;margin-right:5px;border-style:solid;border-width:1px;float:left;\">%s</div></a></li>", kutil_urlencode(user->username), user->avatar, user->username);
    if(user->permissions == 3)
      vert_asprintf(&final_navbar, navbar_html, "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/login/\">LOGOUT</a></li>", "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/manage/\">MANAGEMENT PANEL</a></li>", final_userhtml);
    else
      vert_asprintf(&final_navbar, navbar_html, "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/login/\">LOGOUT</a></li>", "", final_userhtml);

    vert_asprintf(&notification_text_return, notification_html, notification_text);
    vert_asprintf(&return_page, home_html, notification_text_return, final_navbar, time_date.tm_year + 1900);

    free(final_userhtml);
  }
  else
  {
    vert_asprintf(&final_navbar, navbar_html, "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/login/\">LOGIN</a></li>", "", "");
    vert_asprintf(&notification_text_return, notification_html, notification_text);
    vert_asprintf(&return_page, home_html, notification_text_return, final_navbar, time_date.tm_year + 1900);
  }

  khttp_puts(req, return_page);

  //vert_clean_user(user);
  vert_clean_user(user);
  free(final_navbar);
  free(return_page);
  free(notification_text_return);
}

void respond_about(struct kreq *req)
{
  char *return_page = NULL, *notification_text_return = NULL, *final_navbar = NULL, *final_userhtml = NULL;
  time_t time_date_time = time(NULL);
  struct tm time_date = *localtime(&time_date_time);
  user_t *user = NULL;

  khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
  khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_HTML]);
  khttp_head(req, kresps[KRESP_CACHE_CONTROL], "no-cache");
  khttp_head(req, kresps[KRESP_EXPIRES], "Tue, 01 Jun 1999 19:58:02 GMT");
  khttp_body(req);


  if(vert_is_user_logged_in(req, &user) == 2)
  {

    vert_asprintf(&final_userhtml, "<li class=\"navbarli\" style=\"float: right;\"><a class=\"navbarli\" href=\"/user/?u=%s\"><div style=\"usernavbar\"><img src=\"%s\" alt=\"usericon\" style=\"width:14px;height:14px;margin:0px;margin-right:5px;border-style:solid;border-width:1px;float:left;\">%s</div></a></li>", kutil_urlencode(user->username), user->avatar, user->username);
    if(user->permissions == 3)
      vert_asprintf(&final_navbar, navbar_html, "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/login/\">LOGOUT</a></li>", "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/manage/\">MANAGEMENT PANEL</a></li>", final_userhtml);
    else
      vert_asprintf(&final_navbar, navbar_html, "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/login/\">LOGOUT</a></li>", "", final_userhtml);

    vert_asprintf(&notification_text_return, notification_html, notification_text);
    vert_asprintf(&return_page, about_html, notification_text_return, final_navbar, time_date.tm_year + 1900);

    free(final_userhtml);
  }
  else
  {
    vert_asprintf(&final_navbar, navbar_html, "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/login/\">LOGIN</a></li>", "", "");
    vert_asprintf(&notification_text_return, notification_html, notification_text);
    vert_asprintf(&return_page, about_html, notification_text_return, final_navbar, time_date.tm_year + 1900);
  }

  khttp_puts(req, return_page);

  vert_clean_user(user);
  free(final_navbar);
  free(return_page);
  free(notification_text_return);
}

void respond_manage(struct kreq *req)
{
  char *return_page = NULL, *notification_text_return = NULL, *final_navbar = NULL, *final_userhtml = NULL, *generated_html = NULL, *loaded_val = NULL;
  time_t time_date_time = time(NULL);
  struct tm time_date = *localtime(&time_date_time);
  user_t *user = NULL;

  khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
  khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_HTML]);
  khttp_head(req, kresps[KRESP_CACHE_CONTROL], "no-cache");
  khttp_head(req, kresps[KRESP_EXPIRES], "Tue, 01 Jun 1999 19:58:02 GMT");
  khttp_body(req);


  if(vert_is_user_logged_in(req, &user) == 2)
  {

    vert_asprintf(&final_userhtml, "<li class=\"navbarli\" style=\"float: right;\"><a class=\"navbarli\" href=\"/user/?u=%s\"><div style=\"usernavbar\"><img src=\"%s\" alt=\"usericon\" style=\"width:14px;height:14px;margin:0px;margin-right:5px;border-style:solid;border-width:1px;float:left;\">%s</div></a></li>", kutil_urlencode(user->username), user->avatar, user->username);
    if(user->permissions == 3)
      vert_asprintf(&final_navbar, navbar_html, "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/login/\">LOGOUT</a></li>", "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/manage/\">MANAGEMENT PANEL</a></li>", final_userhtml);
    else
      vert_asprintf(&final_navbar, navbar_html, "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/login/\">LOGOUT</a></li>", "", final_userhtml);

      if(user->permissions == 3)
      {
        if(req->method == KMETHOD_POST && req->fieldsz == 3)
        {
          if(strcmp(req->fields[1].val, manage_key) != 0) /* if the key isnt provided */
          {
            vert_asprintf(&notification_text_return, notification_html, notification_text);
            vert_asprintf(&return_page, manage_html, notification_text_return, final_navbar, "invalid management key", req->fields[0].val, "not authorized", time_date.tm_year + 1900);
          }
          else if(req->fields[2].valsz == 0) /* set the user data */
          {
            if(vert_manage_get_value(req->fields[0].val, &loaded_val) != 2)
            {
              vert_asprintf(&notification_text_return, notification_html, notification_text);
              vert_asprintf(&return_page, manage_html, notification_text_return, final_navbar, "either invalid json, invalid user, or db error", req->fields[0].val, "couldn't find", time_date.tm_year + 1900);
            }
            else
            {
              vert_asprintf(&notification_text_return, notification_html, notification_text);
              vert_asprintf(&return_page, manage_html, notification_text_return, final_navbar, "Editing selected user", req->fields[0].val, loaded_val, time_date.tm_year + 1900);
              free(loaded_val);
            }
          }
          else if(vert_post_manage_html(req->fields[0].val, req->fields[2].val) != 2)
          {
            vert_asprintf(&notification_text_return, notification_html, notification_text);
            vert_asprintf(&return_page, manage_html, notification_text_return, final_navbar, "either invalid json, invalid user, or db error", "", "", time_date.tm_year + 1900);
          }
          else
          {
            vert_generate_manage_html(&generated_html);
            vert_asprintf(&notification_text_return, notification_html, notification_text);
            if(generated_html != NULL)
              vert_asprintf(&return_page, manage_html, notification_text_return, final_navbar, generated_html, "", "", time_date.tm_year + 1900);
            else
              vert_asprintf(&return_page, manage_html, notification_text_return, final_navbar, "perfect :/", "", "", time_date.tm_year + 1900);

            if(generated_html != NULL)
              free(generated_html);
          }
        }
        else
        {
          vert_generate_manage_html(&generated_html);
          vert_asprintf(&notification_text_return, notification_html, notification_text);
          if(generated_html != NULL)
            vert_asprintf(&return_page, manage_html, notification_text_return, final_navbar, generated_html, "", "", time_date.tm_year + 1900);
          else
            vert_asprintf(&return_page, manage_html, notification_text_return, final_navbar, "perfect :/", "", "", time_date.tm_year + 1900);

          if(generated_html != NULL)
            free(generated_html);
        }

      }
      else
      {
        vert_asprintf(&notification_text_return, notification_html, notification_text);
        vert_asprintf(&return_page, manage_html, notification_text_return, final_navbar, "<font color=\"red\">Access Denied</font>", "", "", time_date.tm_year + 1900);
      }

    free(final_userhtml);
  }
  else
  {
    vert_asprintf(&final_navbar, navbar_html, "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/login/\">LOGIN</a></li>", "", "");

    vert_asprintf(&notification_text_return, notification_html, notification_text);
    vert_asprintf(&return_page, manage_html, notification_text_return, final_navbar, "<font color=\"red\">Access Denied</font>", "", "", time_date.tm_year + 1900);
  }



  khttp_puts(req, return_page);


  vert_clean_user(user);
  free(final_navbar);
  free(return_page);
  free(notification_text_return);
}

void respond_user(struct kreq *req)
{
  char *return_page = NULL, *notification_text_return = NULL, *final_navbar = NULL, *final_userhtml = NULL, *user_info = NULL, *user_priv = NULL, *user_edit_button = NULL, *file_type = NULL, *user_pfp_loc = NULL, epoch_str[64];
  time_t time_date_time = time(NULL);
  struct tm time_date = *localtime(&time_date_time);
  user_t *user = NULL, *view_user = NULL, temp_user, *useless_user = NULL;
  int can_continue = 1;


  khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
  khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_HTML]);
  khttp_head(req, kresps[KRESP_CACHE_CONTROL], "no-cache");
  khttp_head(req, kresps[KRESP_EXPIRES], "Tue, 01 Jun 1999 19:58:02 GMT");
  khttp_body(req);


  if(vert_is_user_logged_in(req, &user) == 2)
  {

    vert_asprintf(&final_userhtml, "<li class=\"navbarli\" style=\"float: right;\"><a class=\"navbarli\" href=\"/user/?u=%s\"><div style=\"usernavbar\"><img src=\"%s\" alt=\"usericon\" style=\"width:14px;height:14px;margin:0px;margin-right:5px;border-style:solid;border-width:1px;float:left;\">%s</div></a></li>", kutil_urlencode(user->username), user->avatar, user->username);
    if(user->permissions == 3)
      vert_asprintf(&final_navbar, navbar_html, "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/login/\">LOGOUT</a></li>", "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/manage/\">MANAGEMENT PANEL</a></li>", final_userhtml);
    else
      vert_asprintf(&final_navbar, navbar_html, "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/login/\">LOGOUT</a></li>", "", final_userhtml);

    free(final_userhtml);
  }
  else
  {
    vert_asprintf(&final_navbar, navbar_html, "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/login/\">LOGIN</a></li>", "", "");
  }


  if(req->fieldsz > 0 && req->method != KMETHOD_POST)
  {

    if(strcmp(req->fields[0].key, "e") == 0) /* edit page */
    {
      if(user != NULL)
      {
        switch(user->permissions)
        {
          case 0:
            vert_asprintf(&user_priv, "Regular User");
          break;
          case 1:
            vert_asprintf(&user_priv, "<font color=\"blue\">OG</font>");
          break;
          case 2:
            vert_asprintf(&user_priv, "<font color=\"green\">Admin</font>");
          break;
          case 3:
            vert_asprintf(&user_priv, "<i>Owner or something I dunno</i>");
          break;
        }

        vert_asprintf(&notification_text_return, notification_html, notification_text);

        vert_asprintf(&return_page, edit_user, notification_text_return, final_navbar, user->avatar, user->username, user_priv, kutil_epoch2str(user->jointime, epoch_str, sizeof(epoch_str)), "", user->bio, user->website, user->twitter, user->youtube_ch, "AAAAAAAAA", time_date.tm_year + 1900);
        free(user_priv);
      }
      else
      {
        vert_asprintf(&notification_text_return, notification_html, notification_text);

        vert_asprintf(&return_page, edit_user, notification_text_return, final_navbar, "error", "error", "error", "error", "error", "error", "error", "error", "error", "AAAAAAAAA", time_date.tm_year + 1900);
      }
    }
    else /* view page */
    {
      switch(vert_view_user(&view_user, NULL, req->fields[0].val))
      {
        case 0:
          vert_asprintf(&notification_text_return, notification_html, notification_text);
          vert_asprintf(&return_page, user_html, notification_text_return, final_navbar, "/default_usericons/usericon.png", "No user", "No user here", "Never", "No info provided", "", time_date.tm_year + 1900);
        break;
        case 1:
          vert_asprintf(&notification_text_return, notification_html, notification_text);
          vert_asprintf(&return_page, user_html, notification_text_return, final_navbar, "/default_usericons/usericon.png", "ERROR user", "ERROR user here", "ERROR", "ERROR info provided", "", time_date.tm_year + 1900);
        break;
        case 2:
          if(view_user == NULL)
          {
            vert_asprintf(&notification_text_return, notification_html, notification_text);
            vert_asprintf(&return_page, user_html, notification_text_return, final_navbar, "/default_usericons/usericon.png", "ERROR user", "ERROR user here", "ERROR", "ERROR info provided", "", time_date.tm_year + 1900);
          }
          else
          {
            vert_asprintf(&notification_text_return, notification_html, notification_text);

            if(user != NULL)
              if(strcmp(user->username, view_user->username) == 0)
                vert_asprintf(&user_edit_button, "<div class=\"seperator\"></div><form action=\"/user/\" method=\"get\"><input type=\"hidden\" name=\"e\" value=\"%s\"><input type=\"submit\" value=\"EDIT\"></form>", user->username);
              else
                vert_asprintf(&user_edit_button, "");

            switch(view_user->permissions)
            {
              case 0:
                vert_asprintf(&user_priv, "Regular User");
              break;
              case 1:
                vert_asprintf(&user_priv, "<font color=\"blue\">OG</font>");
              break;
              case 2:
                vert_asprintf(&user_priv, "<font color=\"green\">Admin</font>");
              break;
              case 3:
                vert_asprintf(&user_priv, "<i>Owner or something I dunno</i>");
              break;
            }


            if(strlen(view_user->bio) == 0 && strlen(view_user->website) == 0 && strlen(view_user->twitter) == 0 && strlen(view_user->youtube_ch) == 0)
            {
              vert_asprintf(&return_page, user_html, notification_text_return, final_navbar, view_user->avatar, view_user->username, user_priv, kutil_epoch2str(view_user->jointime, epoch_str, sizeof(epoch_str)), "No info provided", (user != NULL) ? user_edit_button:"", time_date.tm_year + 1900);
            }
            else
            {
              /* vert_asprintf(&user_info, "Bio:<div class=\"different_content3\">%s</div><div class=\"seperator_small\"></div>Website: %s<div class=\"seperator_small\"></div>Twitter: %s<div class=\"seperator_small\"></div>YouTube Channel: %s<div class=\"seperator_small\"></div>Birthday: %s", strlen(view_user->bio) ? view_user->bio:"No bio provided", strlen(view_user->website) ? view_user->website:"No website provied", strlen(view_user->twitter) ? view_user->twitter:"No twitter provided", strlen(view_user->youtube_ch) ? view_user->youtube_ch:"No channel provided",  view_user->birthday ? kutil_epoch2str(view_user->birthday, epoch_str, sizeof(epoch_str)):"Not Provided");
              */

              vert_asprintf(&user_info, "Bio:<div class=\"different_content3\">%s</div><div class=\"seperator_small\"></div>Website: %s<div class=\"seperator_small\"></div>Twitter: %s<div class=\"seperator_small\"></div>YouTube Channel: %s", strlen(view_user->bio) ? view_user->bio:"No bio provided", strlen(view_user->website) ? view_user->website:"No website provied", strlen(view_user->twitter) ? view_user->twitter:"No twitter provided", strlen(view_user->youtube_ch) ? view_user->youtube_ch:"No channel provided");


              vert_asprintf(&return_page, user_html, notification_text_return, final_navbar, view_user->avatar, view_user->username, user_priv, kutil_epoch2str(view_user->jointime, epoch_str, sizeof(epoch_str)), user_info, (user != NULL) ? user_edit_button:"", time_date.tm_year + 1900);
              free(user_info);
            }

            free(user_priv);
            if(user != NULL)
              free(user_edit_button);
          }

        break;
      }
    }



  }
  else
  {
    if(req->method == KMETHOD_POST)
    {
      if(user != NULL)
      {
        switch(user->permissions)
        {
          case 0:
            vert_asprintf(&user_priv, "Regular User");
          break;
          case 1:
            vert_asprintf(&user_priv, "<font color=\"blue\">OG</font>");
          break;
          case 2:
            vert_asprintf(&user_priv, "<font color=\"green\">Admin</font>");
          break;
          case 3:
            vert_asprintf(&user_priv, "<i>Owner or something I dunno</i>");
          break;
        }

        vert_asprintf(&notification_text_return, notification_html, notification_text);

        if(strlen(req->fields[0].file) != 0)
        {
          file_type = strrchr(req->fields[0].file, '.');
          file_type++;
        }


        if(req->fieldsz == 6)
        {

          temp_user = *user;
          temp_user.avatar = user->avatar;
          temp_user.username = req->fields[1].val;
          temp_user.bio = req->fields[2].val;
          temp_user.website = req->fields[3].val;
          temp_user.twitter = req->fields[4].val;
          temp_user.youtube_ch = req->fields[5].val;


          if(req->fields[0].valsz > 4194304) /* 4mb */
          {
            vert_asprintf(&return_page, edit_user, notification_text_return, final_navbar, temp_user.avatar, temp_user.username, user_priv, kutil_epoch2str(temp_user.jointime, epoch_str, sizeof(epoch_str)), "<br><font color=\"red\">File too big</font>", temp_user.bio, temp_user.website, temp_user.twitter, temp_user.youtube_ch, "AAAAAAAAA", time_date.tm_year + 1900);
          }
          else
          {
            if(file_type != NULL)
            {
              if(vert_manage_pfp(req->fields[0].val, file_type, user->id, req->fields[0].valsz, &user_pfp_loc))
              {
                vert_asprintf(&return_page, edit_user, notification_text_return, final_navbar, temp_user.avatar, temp_user.username, user_priv, kutil_epoch2str(temp_user.jointime, epoch_str, sizeof(epoch_str)), "<br><font color=\"red\">Invalid file</font>", temp_user.bio, temp_user.website, temp_user.twitter, temp_user.youtube_ch, "AAAAAAAAA", time_date.tm_year + 1900);
                can_continue = 0;
              }
              else
              {
                temp_user.avatar = user_pfp_loc;
                can_continue = 1;
              }
            }
            if(can_continue)
            {
              //if(strlen(temp_user.username) > 42 && temp_user.permissions == 0)
              if(strlen(temp_user.username) > 42)
              {
                vert_asprintf(&return_page, edit_user, notification_text_return, final_navbar, temp_user.avatar, temp_user.username, user_priv, kutil_epoch2str(temp_user.jointime, epoch_str, sizeof(epoch_str)), "<br><font color=\"red\">Username too long</font>", temp_user.bio, temp_user.website, temp_user.twitter, temp_user.youtube_ch, "AAAAAAAAA", time_date.tm_year + 1900);
              }
              else if(strlen(temp_user.bio) > 1024 && temp_user.permissions == 0)
              {
                vert_asprintf(&return_page, edit_user, notification_text_return, final_navbar, temp_user.avatar, temp_user.username, user_priv, kutil_epoch2str(temp_user.jointime, epoch_str, sizeof(epoch_str)), "<br><font color=\"red\">Bio too long</font>", temp_user.bio, temp_user.website, temp_user.twitter, temp_user.youtube_ch, "AAAAAAAAA", time_date.tm_year + 1900);
              }
              else if(strlen(temp_user.website) > 64 && temp_user.permissions == 0)
              {
                vert_asprintf(&return_page, edit_user, notification_text_return, final_navbar, temp_user.avatar, temp_user.username, user_priv, kutil_epoch2str(temp_user.jointime, epoch_str, sizeof(epoch_str)), "<br><font color=\"red\">Website too long</font>", temp_user.bio, temp_user.website, temp_user.twitter, temp_user.youtube_ch, "AAAAAAAAA", time_date.tm_year + 1900);
              }
              else if(strlen(temp_user.twitter) > 64 && temp_user.permissions == 0)
              {
                vert_asprintf(&return_page, edit_user, notification_text_return, final_navbar, temp_user.avatar, temp_user.username, user_priv, kutil_epoch2str(temp_user.jointime, epoch_str, sizeof(epoch_str)), "<br><font color=\"red\">Twitter too long</font>", temp_user.bio, temp_user.website, temp_user.twitter, temp_user.youtube_ch, "AAAAAAAAA", time_date.tm_year + 1900);
              }
              else if(strlen(temp_user.youtube_ch) > 64 && temp_user.permissions == 0)
              {
                vert_asprintf(&return_page, edit_user, notification_text_return, final_navbar, temp_user.avatar, temp_user.username, user_priv, kutil_epoch2str(temp_user.jointime, epoch_str, sizeof(epoch_str)), "<br><font color=\"red\">Youtube channel too long</font>", temp_user.bio, temp_user.website, temp_user.twitter, temp_user.youtube_ch, "AAAAAAAAA", time_date.tm_year + 1900);
              }
              else if(vert_contains_html(temp_user.bio) && (temp_user.permissions == 0 || temp_user.permissions == 3))
              {
                vert_asprintf(&return_page, edit_user, notification_text_return, final_navbar, temp_user.avatar, temp_user.username, user_priv, kutil_epoch2str(temp_user.jointime, epoch_str, sizeof(epoch_str)), "<br><font color=\"red\">Bio contains either '<' or '>'</font>", temp_user.bio, temp_user.website, temp_user.twitter, temp_user.youtube_ch, "AAAAAAAAA", time_date.tm_year + 1900);
              }
              else if(vert_contains_html(temp_user.website) && (temp_user.permissions == 0 || temp_user.permissions == 3))
              {
                vert_asprintf(&return_page, edit_user, notification_text_return, final_navbar, temp_user.avatar, temp_user.username, user_priv, kutil_epoch2str(temp_user.jointime, epoch_str, sizeof(epoch_str)), "<br><font color=\"red\">Website contains either '<' or '>'</font>", temp_user.bio, temp_user.website, temp_user.twitter, temp_user.youtube_ch, "AAAAAAAAA", time_date.tm_year + 1900);
              }
              else if(vert_contains_html(temp_user.username) && (temp_user.permissions == 0 || temp_user.permissions == 3))
              {
                vert_asprintf(&return_page, edit_user, notification_text_return, final_navbar, temp_user.avatar, temp_user.username, user_priv, kutil_epoch2str(temp_user.jointime, epoch_str, sizeof(epoch_str)), "<br><font color=\"red\">Username contains either '<' or '>'</font>", temp_user.bio, temp_user.website, temp_user.twitter, temp_user.youtube_ch, "AAAAAAAAA", time_date.tm_year + 1900);
              }
              else if(vert_contains_html(temp_user.twitter) && (temp_user.permissions == 0 || temp_user.permissions == 3))
              {
                vert_asprintf(&return_page, edit_user, notification_text_return, final_navbar, temp_user.avatar, temp_user.username, user_priv, kutil_epoch2str(temp_user.jointime, epoch_str, sizeof(epoch_str)), "<br><font color=\"red\">Twitter contains either '<' or '>'</font>", temp_user.bio, temp_user.website, temp_user.twitter, temp_user.youtube_ch, "AAAAAAAAA", time_date.tm_year + 1900);
              }
              else if(vert_contains_html(temp_user.youtube_ch) && (temp_user.permissions == 0 || temp_user.permissions == 3))
              {
                vert_asprintf(&return_page, edit_user, notification_text_return, final_navbar, temp_user.avatar, temp_user.username, user_priv, kutil_epoch2str(temp_user.jointime, epoch_str, sizeof(epoch_str)), "<br><font color=\"red\">YouTube channel contains either '<' or '>'</font>", temp_user.bio, temp_user.website, temp_user.twitter, temp_user.youtube_ch, "AAAAAAAAA", time_date.tm_year + 1900);
              }
              else if(vert_view_user(&useless_user, NULL, temp_user.username) != 0 && strcmp(temp_user.username, user->username) != 0)
              {
                vert_asprintf(&return_page, edit_user, notification_text_return, final_navbar, temp_user.avatar, temp_user.username, user_priv, kutil_epoch2str(temp_user.jointime, epoch_str, sizeof(epoch_str)), "<br><font color=\"red\">Username exists</font>", temp_user.bio, temp_user.website, temp_user.twitter, temp_user.youtube_ch, "AAAAAAAAA", time_date.tm_year + 1900);
              }
              else if(vert_edit_user(user->username, user->id, &temp_user) != 2)
              {
                vert_asprintf(&return_page, edit_user, notification_text_return, final_navbar, temp_user.avatar, temp_user.username, user_priv, kutil_epoch2str(temp_user.jointime, epoch_str, sizeof(epoch_str)), "<br><font color=\"red\">Really bad error</font>", temp_user.bio, temp_user.website, temp_user.twitter, temp_user.youtube_ch, "AAAAAAAAA", time_date.tm_year + 1900);
              }
              else
              {
                vert_asprintf(&return_page, edit_user, notification_text_return, final_navbar, temp_user.avatar, temp_user.username, user_priv, kutil_epoch2str(temp_user.jointime, epoch_str, sizeof(epoch_str)), "<br><font color=\"green\">Updated successfully</font>", temp_user.bio, temp_user.website, temp_user.twitter, temp_user.youtube_ch, "AAAAAAAAA", time_date.tm_year + 1900);
              }

            }


          }

        }
        else
          vert_asprintf(&return_page, edit_user, notification_text_return, final_navbar, user->avatar, user->username, user_priv, kutil_epoch2str(user->jointime, epoch_str, sizeof(epoch_str)), "<br><font color=\"red\">TOO FEW FIELDS</font>", user->bio, user->website, user->twitter, user->youtube_ch, "AAAAAAAAA", time_date.tm_year + 1900);

        free(user_priv);
      }
      else
      {
        vert_asprintf(&notification_text_return, notification_html, notification_text);

        vert_asprintf(&return_page, edit_user, notification_text_return, final_navbar, "error", "error", "error", "error", "error", "error", "error", "error", "error", "AAAAAAAAA", time_date.tm_year + 1900);
      }
    }
    else
    {

      vert_asprintf(&notification_text_return, notification_html, notification_text);
      vert_asprintf(&return_page, user_html, notification_text_return, final_navbar, "/default_usericons/usericon.png", "No user", "No user here", "Never", "No info provided", "", time_date.tm_year + 1900);
    }
  }



  khttp_puts(req, return_page);

  if(user_pfp_loc != NULL)
    free(user_pfp_loc);

  if(useless_user != NULL)
    vert_clean_user(useless_user);

  vert_clean_user(view_user);
  vert_clean_user(user);
  free(final_navbar);
  free(return_page);
  free(notification_text_return);
}

void respond_login(struct kreq *req)
{
  char *return_page = NULL, *notification_text_return = NULL, *token= NULL, epoch_str[64], *final_navbar = NULL, *final_userhtml = NULL;
  time_t time_date_time = time(NULL);
  struct tm time_date = *localtime(&time_date_time);
  user_t *user = NULL;
  unsigned int can_cookie = 0, is_loggedin = 0;

  vert_asprintf(&notification_text_return, notification_html, notification_text);

  if(vert_is_user_logged_in(req, &user) == 2)
  {

    vert_asprintf(&final_userhtml, "<li class=\"navbarli\" style=\"float: right;\"><a class=\"navbarli\" href=\"/user/?u=%s\"><div style=\"usernavbar\"><img src=\"%s\" alt=\"usericon\" style=\"width:14px;height:14px;margin:0px;margin-right:5px;border-style:solid;border-width:1px;float:left;\">%s</div></a></li>", kutil_urlencode(user->username), user->avatar, user->username);
    if(user->permissions == 3)
      vert_asprintf(&final_navbar, navbar_html, "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/login/\">LOGOUT</a></li>", "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/manage/\">MANAGEMENT PANEL</a></li>", final_userhtml);
    else
      vert_asprintf(&final_navbar, navbar_html, "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/login/\">LOGOUT</a></li>", "", final_userhtml);

    is_loggedin = 1;

    free(final_userhtml);
  }
  else
  {
    vert_asprintf(&final_navbar, navbar_html, "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/login/\">LOGIN</a></li>", "", "");
  }

  if(req->method == KMETHOD_POST)
  {
    if(req->fieldsz < 2)
    {
      vert_asprintf(&return_page, login_html, notification_text_return, final_navbar, "<font color=\"red\">IMPOSSIBLE USER ERROR(KEY NUM)</font><br>", "<font color=\"red\">IMPOSSIBLE USER ERROR(KEY NUM)</font><br>", time_date.tm_year + 1900);
    }
    else if(strncmp("user", req->fields[0].key, strlen("user")) != 0 || strncmp("pass", req->fields[1].key, strlen("pass")) != 0)
    {
      vert_asprintf(&return_page, login_html, notification_text_return, final_navbar, "<font color=\"red\">IMPOSSIBLE USER ERROR(KEY NAME)</font><br>", "<font color=\"red\">IMPOSSIBLE USER ERROR(KEY NAME)</font><br>", time_date.tm_year + 1900);
    }
    else if(req->fields[0].valsz == 0 || req->fields[1].valsz == 0)
    {
      vert_asprintf(&return_page, login_html, notification_text_return, final_navbar, req->fields[0].valsz ? "":"<font color=\"red\">PLEASE ENTER USERNAME</font><br>", req->fields[1].valsz ? "":"<font color=\"red\">PLEASE ENTER PASSWORD</font><br>", time_date.tm_year + 1900);
    }
    else/* TODO pop up a messagebox and send to home */
    {
      switch(vert_login_user(req->fields[0].val, req->fields[1].val, &token))
      {
        case 0:
          vert_asprintf(&return_page, login_html, notification_text_return, final_navbar, "<font color=\"red\">USER NOT FOUND</font><br>", "", time_date.tm_year + 1900);
        break;
        case 1:
          vert_asprintf(&return_page, login_html, notification_text_return, final_navbar, "<font color=\"red\">IMPOSSIBLE DB ERROR AAAAAAAA</font><br>", "<font color=\"red\">IMPOSSIBLE DB ERROR AAAAAAAA</font><br>", time_date.tm_year + 1900);
        break;
        case 3:
          vert_asprintf(&return_page, login_html, notification_text_return, final_navbar, "", "<font color=\"red\">PASSWORD INCORRECT</font><br>", time_date.tm_year + 1900);
        break;
        default:
          vert_asprintf(&return_page, login_html, notification_text_return, final_navbar, "", "", time_date.tm_year + 1900);
          can_cookie = 1;
      }

    }


  }
  else
  {
    if(is_loggedin)
    {
      vert_asprintf(&return_page, confirm_logout_html, notification_text_return, final_navbar, time_date.tm_year + 1900);
    }
    else
    {
      vert_asprintf(&return_page, login_html, notification_text_return, final_navbar, "", "", time_date.tm_year + 1900);
    }
  }


  if(can_cookie)
  {
    khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_302]);
    khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_HTML]);
    khttp_head(req, kresps[KRESP_CACHE_CONTROL], "no-cache");
    khttp_head(req, kresps[KRESP_EXPIRES], "Tue, 01 Jun 1999 19:58:02 GMT");
    khttp_head(req, kresps[KRESP_LOCATION], "/");
    khttp_head(req, kresps[KRESP_SET_COOKIE], "login_token=%s; Expires=%s; Secure; HttpOnly; Path=/", token, kutil_epoch2str(time(NULL) + 60 * 60 * 24 * 7, epoch_str, sizeof(epoch_str)));
    /* fprintf(stderr, "setting cookie \"login_token=%s; Expires=%s; Secure; HttpOnly; Path=/\"", token, kutil_epoch2str(time(NULL) + 60 * 60 * 24 * 7, epoch_str, sizeof(epoch_str))); */
    free(token);
  }
  else
  {
    khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
    khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_HTML]);
    khttp_head(req, kresps[KRESP_CACHE_CONTROL], "no-cache");
    khttp_head(req, kresps[KRESP_EXPIRES], "Tue, 01 Jun 1999 19:58:02 GMT");
  }
  khttp_body(req);

  khttp_puts(req, return_page);

  vert_clean_user(user);
  free(final_navbar);
  free(return_page);
  free(notification_text_return);
}

void respond_register(struct kreq *req)
{
  char *return_page = NULL, *notification_text_return = NULL, hash[crypto_pwhash_STRBYTES], *final_navbar = NULL, *final_userhtml = NULL;
  time_t time_date_time = time(NULL);
  struct tm time_date = *localtime(&time_date_time);
  user_t *user = NULL;


  vert_asprintf(&notification_text_return, notification_html, notification_text);

  if(vert_is_user_logged_in(req, &user) == 2)
  {

    vert_asprintf(&final_userhtml, "<li class=\"navbarli\" style=\"float: right;\"><a class=\"navbarli\" href=\"/user/?u=%s\"><div style=\"usernavbar\"><img src=\"%s\" alt=\"usericon\" style=\"width:14px;height:14px;margin:0px;margin-right:5px;border-style:solid;border-width:1px;float:left;\">%s</div></a></li>", kutil_urlencode(user->username), user->avatar, user->username);
    if(user->permissions == 3)
      vert_asprintf(&final_navbar, navbar_html, "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/login/\">LOGOUT</a></li>", "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/manage/\">MANAGEMENT PANEL</a></li>", final_userhtml);
    else
      vert_asprintf(&final_navbar, navbar_html, "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/login/\">LOGOUT</a></li>", "", final_userhtml);

    free(final_userhtml);
  }
  else
  {
    vert_asprintf(&final_navbar, navbar_html, "<li class=\"navbarli\"><a class=\"navbarli\" href=\"/login/\">LOGIN</a></li>", "", "");
  }

  if(req->method == KMETHOD_POST)
  {
    if(req->fieldsz < 4)
    {
      vert_asprintf(&return_page, register_html, notification_text_return, final_navbar, "<font color=\"red\">IMPOSSIBLE USER ERROR(KEY NUM)_NOTICE - BOTS, PLEASE DO NOT TRY TO AUTO MAKE ACCOUNTS</font><br><br>", "<font color=\"red\">IMPOSSIBLE USER ERROR(KEY NUM)</font><br>", "<font color=\"red\">IMPOSSIBLE USER ERROR(KEY NUM)</font><br>", "<font color=\"red\">IMPOSSIBLE USER ERROR(KEY NUM)</font><br>", "<font color=\"red\">IMPOSSIBLE USER ERROR(KEY NUM)</font><br>", time_date.tm_year + 1900);
    }
    else if(strncmp("email", req->fields[0].key, strlen("email")) != 0 || strncmp("user", req->fields[1].key, strlen("user")) != 0 || strncmp("pass", req->fields[2].key, strlen("pass")) != 0 || strncmp("passv", req->fields[3].key, strlen("passv")) != 0)
    {
      vert_asprintf(&return_page, register_html, notification_text_return, final_navbar, "<font color=\"red\">IMPOSSIBLE USER ERROR(KEY VAL)_NOTICE - BOTS, PLEASE DO NOT TRY TO AUTO MAKE ACCOUNTS</font><br><br>", "<font color=\"red\">IMPOSSIBLE USER ERROR(KEY VAL)</font><br>", "<font color=\"red\">IMPOSSIBLE USER ERROR(KEY VAL)</font><br>", "<font color=\"red\">IMPOSSIBLE USER ERROR(KEY VAL)</font><br>", "<font color=\"red\">IMPOSSIBLE USER ERROR(KEY VAL)</font><br>", time_date.tm_year + 1900);
    }
    else if(req->fields[0].valsz == 0 || req->fields[1].valsz == 0 || req->fields[2].valsz == 0 || req->fields[3].valsz == 0)
    {
      vert_asprintf(&return_page, register_html, notification_text_return, final_navbar, "<font color=\"red\">PLEASE FILL OUT ALL FIELDS</font><br><br>", req->fields[0].valsz ? "":"<font color=\"red\">PLEASE ENTER EMAIL</font><br>", req->fields[1].valsz ? "":"<font color=\"red\">PLEASE ENTER USERNAME</font><br>", req->fields[2].valsz ? "":"<font color=\"red\">PLEASE ENTER A PASSWORD</font><br>", req->fields[3].valsz ? "":"<font color=\"red\">PLEASE ENTER THE PASSWORD A SECOND TIME</font><br>", time_date.tm_year + 1900);
    }
    else if(strchr(req->fields[0].val, '@') == NULL || strchr(req->fields[0].val, '.') == NULL)
    {
      vert_asprintf(&return_page, register_html, notification_text_return, final_navbar, "<font color=\"red\">INVALID EMAIL</font><br><br>", "", "", "", "", time_date.tm_year + 1900);
    }
    else if(strcmp(req->fields[2].val, req->fields[3].val) != 0)
    {
      vert_asprintf(&return_page, register_html, notification_text_return, final_navbar, "<font color=\"red\">PASSWORDS DO NOT MATCH</font><br><br>", "", "", "", "", time_date.tm_year + 1900);
    }
    else if(strlen(req->fields[0].val) > 128)
    {
      vert_asprintf(&return_page, register_html, notification_text_return, final_navbar, "<font color=\"red\">EMAIL TOO LONG(128 chars max)</font><br><br>", "", "", "", "", time_date.tm_year + 1900);
    }
    else if(strlen(req->fields[1].val) > 42)
    {
      vert_asprintf(&return_page, register_html, notification_text_return, final_navbar, "<font color=\"red\">USERNAME TOO LONG(42 chars max)</font><br><br>", "", "", "", "", time_date.tm_year + 1900);
    }
    else if(strlen(req->fields[2].val) > 128)
    {
      vert_asprintf(&return_page, register_html, notification_text_return, final_navbar, "<font color=\"red\">PASSWORD TOO LONG(128 chars max)</font><br><br>", "", "", "", "", time_date.tm_year + 1900);
    }
    else if(vert_contains_html(req->fields[0].val))
    {
      vert_asprintf(&return_page, register_html, notification_text_return, final_navbar, "<font color=\"red\">CANT HAVE ANY '<''>' IN THE EMAIL</font><br><br>", "", "", "", "", time_date.tm_year + 1900);
    }
    else if(vert_contains_html(req->fields[1].val))
    {
      vert_asprintf(&return_page, register_html, notification_text_return, final_navbar, "<font color=\"red\">CANT HAVE ANY '<''>' IN THE USERNAME</font><br><br>", "", "", "", "", time_date.tm_year + 1900);
    }
    else /* TODO refer to home */
    {
      crypto_pwhash_str(hash, req->fields[2].val, req->fields[2].valsz, crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE);


      switch(vert_create_user(req->fields[1].val, req->fields[0].val, hash, req->remote))
      {
        case 1: /* general db error */
          vert_asprintf(&return_page, register_html, notification_text_return, final_navbar, "VERY BAD DB ERROR. UH OH<br><br>", "", "", "", "", time_date.tm_year + 1900);
        break;
        case 2: /* user exists */
          vert_asprintf(&return_page, register_html, notification_text_return, final_navbar, "USERNAME TAKEN<br><br>", "", "", "", "", time_date.tm_year + 1900);
        break;
        case 3: /* email taken */
          vert_asprintf(&return_page, register_html, notification_text_return, final_navbar, "EMAIL TAKEN<br><br>", "", "", "", "", time_date.tm_year + 1900);
        break;
        default:
          vert_asprintf(&return_page, register_html, notification_text_return, final_navbar, "EMAIL SENT. IF THE EMAIL ENTERED WAS WRONG, ENTER A NEW EMAIL. CLICK TO RESEND EMAIL. CHECK YOUR SPAM<br><br>", "", "", "", "", time_date.tm_year + 1900);
      }

    }

  }
  else
    vert_asprintf(&return_page, register_html, notification_text_return, final_navbar, "", "", "", "", "", time_date.tm_year + 1900);

  khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
  khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[KMIME_TEXT_HTML]);
  khttp_head(req, kresps[KRESP_CACHE_CONTROL], "no-cache");
  khttp_head(req, kresps[KRESP_EXPIRES], "Tue, 01 Jun 1999 19:58:02 GMT");
  khttp_body(req);

  khttp_puts(req, return_page);

  vert_clean_user(user);
  free(final_navbar);
  free(return_page);
  free(notification_text_return);
}

void respond_status(struct kreq *req)
{

}

void update_file(char *path)
{
  recent_mod = path;
  if(strcmp("home.html", path) == 0)
  {
    free(home_html);
    home_html = vert_read_file(CGI_ROOT "home.html", &home_html_len);
  }
  else if(strcmp("navbar.html", path) == 0)
  {
    free(navbar_html);
    navbar_html = vert_read_file(CGI_ROOT "navbar.html", &navbar_html_len);
  }
  else if(strcmp("notification.html", path) == 0)
  {
    free(notification_html);
    notification_html = vert_read_file(CGI_ROOT "notification.html", &notification_html_len);
  }
  else if(strcmp("notifications.txt", path) == 0)
  {
    free(notification_text);
    notification_text = vert_read_file(CGI_ROOT "notifications.txt", &notification_text_len);
  }
  else if(strcmp("about.html", path) == 0)
  {
    free(about_html);
    about_html = vert_read_file(CGI_ROOT "about/about.html", &about_html_len);
  }
  else if(strcmp("login.html", path) == 0)
  {
    free(login_html);
    login_html = vert_read_file(CGI_ROOT "login/login.html", &login_html_len);
  }
  else if(strcmp("register.html", path) == 0)
  {
    free(register_html);
    register_html = vert_read_file(CGI_ROOT "register/register.html", &register_html_len);
  }
  else if(strcmp("confirm_logout.html", path) == 0)
  {
    free(confirm_logout_html);
    confirm_logout_html = vert_read_file(CGI_ROOT "confirm_logout/confirm_logout.html", &confirm_logout_html_len);
  }
  else if(strcmp("user.html", path) == 0)
  {
    free(user_html);
    user_html = vert_read_file(CGI_ROOT "user/user.html", &user_html_len);
  }
  else if(strcmp("edit_user.html", path) == 0)
  {
    free(edit_user);
    edit_user = vert_read_file(CGI_ROOT "user/edit_user.html", &edit_user_len);
  }
  else if(strcmp("manage.html", path) == 0)
  {
    free(manage_html);
    manage_html = vert_read_file(CGI_ROOT "manage.html", &manage_html_len);
  }

}

void *u_thread(void *ptr)
{
  int i = *(int *)ptr;
  while(!vert_shutdown)
  {
    vert_check_change(&fd[i], update_file);
  }
}


void route()
{
  struct kreq req;

  while(khttp_fcgi_parse(fcgi, &req) == KCGI_OK && !vert_shutdown)
  {
    fprintf(stderr, "connection from %s requesting %s\n", req.remote, req.pagename); /* temporary to tell when it hangs */
    /* route requests */
    if(strcmp(req.pagename, "") == 0)
      respond_home(&req);
    else if(strncmp(req.pagename, "about", strlen("about")) == 0)
      respond_about(&req);
    else if(strncmp(req.pagename, "login", strlen("login")) == 0)
      respond_login(&req);
    else if(strncmp(req.pagename, "register", strlen("register")) == 0)
      respond_register(&req);
    else if(strncmp(req.pagename, "confirm_logout", strlen("confirm_logout")) == 0)
      respond_confirmlogout(&req);
    else if(strncmp(req.pagename, "user", strlen("user")) == 0)
      respond_user(&req);
    else if(strncmp(req.pagename, "manage", strlen("manage")) == 0)
      respond_manage(&req);
    else /* last resort if it doesn't find anything */
      respond_debug(&req);
    /* ============= */

    khttp_free(&req);
  }

}

short destroy()
{
  khttp_fcgi_free(fcgi);
  pthread_join(file_update_thread, NULL);
  unqlite_close(acc_db);
  unqlite_close(post_db);
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
    printf("cleanly exitted\n");
  return 0;
}
