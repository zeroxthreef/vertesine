#include "util.h"
#include "cache.h"
#include "templating.h"
#include "pages/pages.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>


int vert_respond(sb_Event *e);

int vert_init(sb_Server **server, sb_Options options);

void vert_destroy(sb_Server *server);

int main(int argc, char **argv);

/* ========================functions============== */

void sig_handler(int sig)
{
	if(sig == SIGINT)
		map_set(&vert_settings, "state", VERT_STATE_SHUTDOWN);
}

int vert_respond(sb_Event *e)
{
	if(e->type == SB_EV_REQUEST)
	{
		log_info("[NETWORKING]:[IP][%s]|[METHOD][%s]|[REQUEST][%s]", e->address, e->method, e->path);
		
		return vert_pages_route(e);
	}
	
	return SB_RES_OK;
}

int vert_init(sb_Server **server, sb_Options options)
{
	ini_t *config = NULL;
	int temp;
	char *temp_s = NULL, directory[PATH_MAX];
	
	log_file = NULL;
	
	log_info("starting server");
	
	signal(SIGINT, &sig_handler);
	
	/* save all of the settings */
	vert_util_write_default_config();
	
	map_init(&vert_settings);
	
	config = ini_load("vertesine.ini");
	
	ini_sget(config, "vertesine", "port", "%d", &temp);
	
	map_set(&vert_settings, "port", temp);
	
	ini_sget(config, "vertesine", "chrooted", "%d", &temp);
	
	map_set(&vert_settings, "chroot", temp);
	
	ini_sget(config, "vertesine", "max_cache_mb", "%d", &temp);
	
	map_set(&vert_settings, "cache", temp);
	
	map_set(&vert_settings, "state", VERT_STATE_RUNNING);
	
	ini_sget(config, "vertesine", "log_path", NULL, &temp_s);
	
	/* setup logging */
	log_file = fopen(temp_s, "a");
	log_set_fp(log_file);
	
	/* vert_util_safe_free(temp_s); */
	temp_s = NULL;
	
	/* continue reading the ini file */
	ini_sget(config, "vertesine", "max_filechunk_mb", "%d", &temp);
	
	map_set(&vert_settings, "filechunk", temp);
	
	ini_sget(config, "vertesine", "sleep", "%d", &temp);
	
	map_set(&vert_settings, "sleep", temp);
	
	ini_sget(config, "vertesine", "elem_per_page", "%d", &temp);
	
	map_set(&vert_settings, "elem_per_page", temp);
	
	map_set(&vert_settings, "current_cache_mb", 0);
	
	ini_sget(config, "database", "port", "%d", &temp);
	
	map_set(&vert_settings, "db_port", temp);
	
	ini_sget(config, "database", "database_id", "%d", &temp);
	
	map_set(&vert_settings, "database_id", temp);
	
	ini_sget(config, "database", "login_token_ttl", "%d", &temp);
	
	map_set(&vert_settings, "token_ttl", temp);
	
	ini_sget(config, "database", "login_token_length", "%d", &temp);
	
	map_set(&vert_settings, "token_length", temp);
	
	ini_free(config);
	/* end of using the ini file ============== */
	
	/* send to chroot */
	if(*map_get(&vert_settings, "chroot"))
	{
		log_info("chrooting...");
		
		if(!getcwd(directory, PATH_MAX))
		{
			log_fatal("couldn't get the current working directory");
			return 1;
		}
		vert_util_asprintf(&temp_s, "%s", directory);
		chdir(temp_s);
		
		/* puts(temp_s); */
		
		if(chroot(temp_s) == -1)
		{
			log_fatal("chroot failed. Aborting");
			return 1;
		}
		
		log_info("chrooted to %s", temp_s);
		
		vert_util_safe_free(temp_s);
		temp_s = NULL;
	}
	else
		log_warn("not chrooted. Proceed with caution");
	
	
	/* init the filecache */
	vert_filecache_init(*map_get(&vert_settings, "cache"));
	
	/* init templating */
	vert_templ_init();
	
	/* init page routing and templating */
	vert_pages_init();
	
	/* setup sandbird */
	
	vert_util_asprintf(&temp_s, "%d", *map_get(&vert_settings, "port"));
	memset(&options, 0, sizeof(options));
	
	options.port = temp_s;
	options.handler = vert_respond;
	
	/* vert_util_safe_free(temp_s); */
	
	if((*server = sb_new_server(&options)) == NULL)
	{
		log_fatal("server could not be started. Check port");
		return 1;
	}
	
	snowflake_init(0, 0); /* TODO make the worker ID change on every new sevrer process */
	
	log_info("server started on port %s. Listening for connections", options.port);
	return 0;
}

void vert_destroy(sb_Server *server)
{
	log_info("server shutting down");
	
	/* close the custom things */
	vert_pages_destroy();
	
	/* close sandbird */
	sb_close_server(server);
	
	/* destroy the filecache */
	vert_filecache_destroy();
	
	/* destroy the templating system */
	vert_templ_destroy();
	
	/* destroy the settings hashmap */
	map_deinit(&vert_settings);
	
	log_info("goodbye");
	
	fclose(log_file);
}

int main(int argc, char **argv)
{
	sb_Server *server = NULL;
	sb_Options options;
	int ret = 0;
	
	if(vert_init(&server, options))
	{
		log_fatal("unable to start");
	}
	else
		while(*map_get(&vert_settings, "state") == VERT_STATE_RUNNING)
		{
			/* log_trace("polling"); */
			sb_poll_server(server, *map_get(&vert_settings, "sleep"));
			vert_filecache_listen();
		}
	
	ret = (*map_get(&vert_settings, "state") == VERT_STATE_UPDATING) ? 1 : 0; /* tell the manager that the server is updating */
	
	vert_destroy(server);
	
	return ret;
}