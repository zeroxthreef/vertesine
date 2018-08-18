#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include <kcgi.h>
#include <sodium.h>
#include <curl/curl.h>
#include <b64/cencode.h>
#include <gd.h>
#include <sys/stat.h>

#define IN_BSIZE (1024 * (sizeof(struct inotify_event) + 16))


void *callback_arr = NULL;
unsigned long callback_count = 0;


short vert_asprintf(char **string, const char *fmt, ...)
{
  va_list list;
  char *tempString = NULL;
  char *oldstring = NULL;
  int size = 0;

  if(*string != NULL)
  {
    //free(*string);
    oldstring = *string;
  }

  va_start(list, fmt);
  size = vsnprintf(tempString, 0, fmt, list);
  va_end(list);
  va_start(list, fmt);

  if((tempString = malloc(size + 1)) != NULL)
  {
    if(vsnprintf(tempString, size + 1, fmt, list) != -1)
    {
      *string = tempString;
      if(oldstring != NULL)
      {
        free(oldstring);
      }
      return size;
    }
    else
    {
      *string = NULL;
      if(oldstring != NULL)
      {
        free(oldstring);
      }
      return -1;
    }
  }
  va_end(list);
}

unsigned char *vert_read_file(const char *location, unsigned long *size_ptr) /* not entirely fit for binary reading since it adds a null terminator to the end */
{
  FILE *f;
  unsigned long size;
  unsigned char *dataPtr;
  f = fopen(location, "rb");
  if(f == NULL)
  {
    fprintf(stderr,"Error cant find %s\n", location);
    return (unsigned char *)NULL;
  }

  fseek(f,0,SEEK_END);
  size = ftell(f);
  rewind(f);

  dataPtr = (unsigned char *)malloc(size + 1);

  fread(dataPtr, sizeof(unsigned char), size, f);

  if(dataPtr == NULL)
  {
    fprintf(stderr, "Outta memory\n");
    return (unsigned char *)NULL;
  }

  fclose(f);
  *size_ptr = size;

  dataPtr[size] = 0x00;

  return dataPtr;
}

int vert_add_inotify_watch(int *fd, int *watch_dir, char *dir)
{
  *fd = inotify_init();

  if(*fd < 0)
  {
    fprintf(stderr, "couldn't init inotify fd\n");
    return 1;
  }

  *watch_dir = inotify_add_watch(*fd, dir, IN_MODIFY | IN_CREATE | IN_DELETE);
  return 0;
}

void vert_check_change(int *fd, void(*vert_ch_callback)(char *path))
{
  long len, i = 0;
  char buf[IN_BSIZE];

  len = read(*fd, buf, IN_BSIZE);

  if(len >= 0)
  {

    while(i < len)
    {
      struct inotify_event *ev = (struct inotify_event *)&buf[i];

      if(ev->len)
      {
        if(ev->mask & IN_CREATE || ev->mask & IN_MODIFY || ev->mask & IN_DELETE)
          vert_ch_callback(ev->name);
      }

      i += sizeof(struct inotify_event) + ev->len;
    }

  }
}

void vert_clean_inotify(int *fd, int *watch_dir)
{
  inotify_rm_watch(*fd, *watch_dir);
  close(*fd);
}

char *vert_generate_random_string(unsigned long maxlen)
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

  sleep(3);

  free(temp_buf);


  return final;
}

int vert_manage_pfp(unsigned char *data, char *extension, char *user_id, unsigned long size, char **pfp_loc)
{
  gdImagePtr pfp;
  FILE *write;
  char *writepath = NULL, *urlpath = NULL;

  vert_asprintf(&writepath, HTML_ROOT "user_av/%s.%s", user_id, extension); /* 100% not letting the filenames be the username. Path traversal would suck */
  vert_asprintf(&urlpath, "/user_av/%s.%s", user_id, extension);

  if(strcmp(extension, "png") == 0)
  {
    pfp = gdImageCreateFromPngPtr((int)size, data);
    if(pfp == NULL)
      return 1;

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

void vert_filter_jansson_breakers(char *string, unsigned long terminated_len)
{
  /*
  unsigned long i;
  for(i = 0; i < terminated_len; i++)
  {
    if(string[i] == '%')
    {
      string[i] = '/';
      fprintf(stderr, "correcting string\n");
    }
  }
  */
}

void vert_filter_html(char *string, unsigned long terminated_len)
{
  unsigned long i;
  for(i = 0; i < terminated_len; i++)
  {
    if(string[i] == '<')
    {
      string[i] = '(';
    }
    if(string[i] == '>')
    {
      string[i] = ')';
    }
  }
}


int vert_contains_html(char *string)
{
  unsigned long i;
  for(i = 0; i < strlen(string); i++)
  {
    if(string[i] == '<')
    {
      return 1;
    }
    if(string[i] == '>')
    {
      return 2;
    }
  }
  return 0;
}

int vert_parse_markdown(char *string, char **output)
{

  return 0;
}

char *vert_create_page_list(unsigned int current, unsigned int max, char *query_root)///entries/
{
  char *final = NULL;
  char *html = "<div class=\"entries_pages_contrainer\">| \
  <div style=\"float: left; display: block; margin-right: 1%%;\">Page|</div> \
  %s \
  </div> \
  <div class=\"seperator\"></div>";
  char *number_html = "<a href=\"%s?p=%lu\"><div class=\"entries_pages_contrainer_numbers_%sactive\">%lu</div></a>";
  char *temp_numlist = NULL, *temp_number = NULL;
  unsigned long i;


  for(i = 0; i < max; i++)
  {
    vert_asprintf(&temp_number, number_html, query_root, i + 1, (i == current) ? "" : "in", i + 1);
    vert_asprintf(&temp_numlist, "%s\n%s", i ? temp_numlist : "", temp_number);

  }

  vert_asprintf(&final, html, temp_numlist);


  free(temp_number);
  free(temp_numlist);

  return final;
}

unsigned long vert_get_db_keyv_num(void *db)
{
  unqlite *temp_db = (unqlite *)db;
  unsigned long count = 0;
  unqlite_kv_cursor *cur;


  if(unqlite_kv_cursor_init(temp_db, &cur) != UNQLITE_OK)
  {
    fprintf(stderr, "cant allocate a cursor\n");
    return 1;
  }

  unqlite_kv_cursor_first_entry(cur);

  while(unqlite_kv_cursor_valid_entry(cur))
  {
    unqlite_kv_cursor_next_entry(cur);
    count++;
  }

  unqlite_kv_cursor_release(temp_db, cur);


  return count;
}

int vert_create_html_dir_with_cgi_index(char *path)
{
  //HTML_ROOT
  int ret;
  char *final = NULL;
  FILE *f = NULL;


  vert_asprintf(&final, "%s%s", HTML_ROOT, path);
  if((ret = mkdir(final, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) != 0)
  {
    fprintf(stderr, "dir creation error at [%s]\n", final);
    free(final);
    return ret;
  }

  fprintf(stderr, "creating dir at %s\n", final);

  vert_asprintf(&final, "%s/%s", final, "index.cgi");
  f = fopen(final, "wb");

  if(f != NULL)
    fclose(f);
  else
    fprintf(stderr, "file creation error\n");

  fprintf(stderr, "creating index at %s\n", final);


  free(final);
  return ret;
}

char *vert_replace(char *original, char *find, char *replace)
{
  char *final = NULL, *begining = NULL;


  if(original != NULL)
  if((begining = strstr(original, find)) != NULL)
  {
    final = calloc(1, (strlen(original) - strlen(find)) + strlen(replace) + 1);

    memmove(final, original, (begining - original));
    memmove(final + strlen(final), replace, strlen(replace));
    memmove(final + strlen(final), original + ((begining - original) + strlen(find)), strlen(original + ((begining - original) + strlen(find))));

    return final;
  }

  return NULL;
}

char *vert_replace_all(char *original, char *find, char *replace)
{
  char *final = NULL, *last = NULL;

  if(original != NULL)
  while((final = vert_replace(final ? final : original, find, replace)) != NULL)
  {
    if(last != NULL)
      free(last);

    last = final;
  }

  return last;
}

int vert_replace_buffer_all(char **original, char *find, char *replace)
{
  char *final = NULL;


  if(original != NULL && *original != NULL)
  if((final = vert_replace_all(*original, find, replace)) != NULL)
  {
    free(*original);
    *original = final;

    return 1;
  }

  return 0;
}

char *vert_strdup(char *original)
{
  char *duplicate = NULL;

  if(original != NULL)
    if((duplicate = calloc(strlen(original) + 1, sizeof(char))) != NULL)
      strcpy(duplicate, original);

  return duplicate;
}

int vert_send_email(char *address, char *subject, char *email_sender, char *email_message) /* TODO do this later */
{
  CURL *curl;
  CURLcode r = CURLE_OK;




  return 0;
}

void vert_display_message(char *template, char *title, char *description, char *newurl, int delay) /* for whole page messages */
{

}
