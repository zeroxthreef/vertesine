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
