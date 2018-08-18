#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>



int can_exit = 0;


void *manage_home(void *ptr)
{
  FILE *fp = NULL;
  char output[24];

  while(!can_exit)
  {
    printf("starting home\n");
    /* system("spawn-fcgi -p 8000 -n HOME_BINARY"); */
    if(fp != NULL)
    {
      while(fgets(output, sizeof(output), fp) != NULL)
      {
        puts(output);
      }
      fp = NULL;
    }
    else
    {
      fp = popen("spawn-fcgi -p 8000 -n HOME_BINARY", "r");
    }
    printf("home crashed. restarting\n");
    sleep(1);
  }
}

void *manage_comments(void *ptr)
{
  FILE *fp = NULL;
  char output[24];

  while(!can_exit)
  {
    printf("starting comments\n");
    /* system("spawn-fcgi -p 8000 -n COMMENTS_BINARY"); */
    if(fp != NULL)
    {
      while(fgets(output, sizeof(output), fp) != NULL)
      {
        puts(output);
        if(strstr(output, "FastCGI"))
          break;
      }
      fp = NULL;
    }
    else
    {
      fp = popen("spawn-fcgi -p 8001 -n COMMENTS_BINARY", "r");
    }
    printf("comments crashed. restarting\n");
    sleep(1);
  }
}

void handle(int signal)
{
  if(signal == SIGINT)
  {
    can_exit = 1;
    printf("exitting\n");
  }
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

int main(int argc, char **argv)
{
  pthread_t home, comments;

  printf("starting manager\n");

  pthread_create(&home, NULL, manage_home, NULL);
  pthread_create(&home, NULL, manage_comments, NULL);

  signal(SIGINT, handle);


  printf("manager started\n");


  while(!can_exit)
  {
    sleep(1);
  }

  return 0;
}
