/* for templates and specific page responses */
/* NOTE DO NOT DELETE THIS FILE. IT IS NEEDED REGARDLESS OF CUSTOMIATIONS */
#include "pages.h"
#include "../util.h"
#include "../cache.h"
#include "../templating.h"

/* the main customization file */
#include "vertesine_custom_templates.h"


/* some of the smaller template callbacks */

static char *internal_test_tag(char *key, sb_Event *e)
{
	return vert_util_strdup("templating system ok");
}

static char *internal_taglist_tag(char *key_sel, sb_Event *e)
{
	char *final_str = NULL, *return_str = NULL, *key = NULL, *temp_str = NULL;
	map_iter_t i = map_iter(&template_callbacks);
	
	vert_util_asprintf(&final_str, "<table><tr><th>tag</th><th>address</th></tr>\n<tr><td>&#91;template_millis]</td><td>(post template generated)</td></tr>\n");
	
	while((key = map_next(&template_callbacks, &i)) != NULL)
	{
		/* key + 1 to remove recursive halt. Adding the [ as an escape */
		vert_util_asprintf(&final_str, "%s<tr><td>&#91;%s</td><td>%p</td></tr>\n", final_str, key + 1, *map_get(&template_callbacks, key));
	}
	vert_util_asprintf(&final_str, "%s</table>\n", final_str);

	return final_str;
}

static char *internal_settinglist_tag(char *key_sel, sb_Event *e)
{
	char *final_str = NULL, *return_str = NULL, *key = NULL, *temp_str = NULL;
	map_iter_t i = map_iter(&vert_settings);
	
	vert_util_asprintf(&final_str, "<table><tr><th>key</th><th>value</th></tr>\n");
	
	while((key = map_next(&vert_settings, &i)) != NULL)
	{
		/* key + 1 to remove recursive halt. Adding the [ as an escape */
		vert_util_asprintf(&final_str, "%s<tr><td>%s</td><td>%d</td></tr>\n", final_str, key, *map_get(&vert_settings, key));
	}
	vert_util_asprintf(&final_str, "%s</table>\n", final_str);

	return final_str;
}

/* ====================================== */

void vert_pages_error(int error, sb_Event *e)
{
	unsigned long file_size = 0;
	uint8_t *template_buffer = NULL;
	double start_time = vert_util_get_ms();
	char *temp_str = NULL;
	
	if((template_buffer = vert_filecache_read("html/error.htmpl", &file_size, VERT_CACHE_TEXT)) != NULL)
		{
			vert_templ_run(&template_buffer, e);
			
			/* TODO add ranges support to generated content */
			switch(error)
			{
				case 404:
					sb_send_status(e->stream, error, "Not Found");
				break;
				default:
					error = 500;
					sb_send_status(e->stream, 500, "Internal Server Error");
			}
			
			/* if there is a [templ_millis], then insert the milliseconds it took to run */
			vert_util_asprintf(&temp_str, "%.1fms", vert_util_get_ms() - start_time);
			vert_util_replace_buffer_all(&template_buffer, "[template_millis]", temp_str);
			
			vert_util_asprintf(&temp_str, "%d", error);
			vert_util_replace_buffer_all(&template_buffer, "[error]", temp_str);
		
			/* end of templating */
			
			
			sb_send_header(e->stream, "Content-Type", vert_util_mime("error.htmpl"));
			sb_write(e->stream, template_buffer, strlen(template_buffer));
	
			/* TODO remove this because the cache needs a file free function. This would break the caching */
			vert_util_safe_free(template_buffer);
		}
		
		vert_util_safe_free(temp_str);
}

void vert_pages_init()
{
	log_info("adding template responses and page responses");
	/* the default tags and functions */
	vert_templ_add("[template_test]", &internal_test_tag);
	vert_templ_add("[template_table]", &internal_taglist_tag);
	vert_templ_add("[server_settings]", &internal_settinglist_tag);
	/* the customization function. One of 2 things to change on reuse */
	vert_custom_templ_add();
	
	vert_custom_init();
	
}

void vert_pages_destroy()
{
	vert_custom_destroy();
}

/* for custom paths that arent on disk */
char *vert_pages_virtual_route(char *path)
{
	char *path_return = NULL, *temp_str = NULL;
	
	if(strncmp(path, "/users/edit/", strlen("/users/edit/")) == 0 && strlen(path) > strlen("/users/edit/")) /* check for subdirectories first */
	{
		/* TODO check if the user exists here so a 404 gets sent if not there */
		
		if(vert_custom_check_user_exists(path + strlen("/users/edit/")))
		{
			path_return = vert_util_strdup("html/user_edit.htmpl");
		}
		else
			path_return = vert_util_strdup("html/this_will_error.htmpl");
			
	}
	else if(strncmp(path, "/users/", strlen("/users/")) == 0 && strlen(path) > strlen("/users/")) /* anything under users that isnt /users or /users/ */
	{
		/* TODO check if the user exists here so a 404 gets sent if not there */
		
		if(vert_custom_check_user_exists(path + strlen("/users/")))
		{
			path_return = vert_util_strdup("html/user_view.htmpl");
		}
		else
			path_return = vert_util_strdup("html/this_will_error.htmpl");
	}
	else /* do not let anyone access the templates if it is not an index */
	{
		if(strstr(path, ".htmpl"))
		{
			if(!(temp_str = strrchr(path, '/')))
				temp_str = path;
			else
				temp_str++;
			
			if(!strcmp(temp_str, "index.htmpl") == 0)
				path_return = vert_util_strdup("html/this_will_error.htmpl");
		}
	}
	
	return path_return;
}

/* TODO determine if there should be virtual pages like /status or if there should an index but heavilt templated */
int vert_pages_route(sb_Event *e) /* probably useless for specifics because I can just throw a specific tag into the template and it will know what page is what */
{
	unsigned long file_size = 0;
	double start_time = vert_util_get_ms();
	uint8_t *template_buffer = NULL;
	char *final_path = NULL, *temp_str = NULL;
	
	/* determine the index from path or file, call the templating system if its a template */
	if((final_path = vert_pages_virtual_route(e->path))){} /* NOTE: this is supposed to be formatted like this */
	else if(!strrchr(e->path, '.')) /* fix path to use index file if path wanted */
	{
		if(e->path[strlen(e->path) - 1] == '/')
		{
			vert_util_asprintf(&final_path, "html%sindex.htmpl", e->path); /* TODO make html files work as indexes aswell */
		}
		else
		{
			vert_util_asprintf(&final_path, "html%s/index.htmpl", e->path); /* TODO make html files work as indexes aswell */
		}
	}
	else /* still need to use the final path string */
		vert_util_asprintf(&final_path, "html%s", e->path);
	
	log_trace("path calculated \"%s\"", final_path);
	
	if(strcmp(strrchr(final_path, '.'), ".htmpl") == 0) /* special template file */
	{
		if((template_buffer = vert_filecache_read(final_path, &file_size, VERT_CACHE_TEXT)) != NULL)
		{
			vert_templ_run(&template_buffer, e);
			
			/* if there is a [templ_millis], then insert the milliseconds it took to run */
			vert_util_asprintf(&temp_str, "%.1fms", vert_util_get_ms() - start_time);
			vert_util_replace_buffer_all(&template_buffer, "[template_millis]", temp_str);
		
			/* end of templating */
			
			/* TODO add ranges support to generated content */
			sb_send_status(e->stream, 200, "OK");
			
			vert_custom_set_get_cookie(e, NULL, 1);
			
			sb_send_header(e->stream, "Content-Type", vert_util_mime(final_path));
			sb_write(e->stream, template_buffer, strlen(template_buffer));
	
			/* TODO remove this because the cache needs a file free function. This would break the caching */
			/* keep because templates read by this function are copied, they have to */
			vert_util_safe_free(template_buffer);
		}
		else /* file does not exist */
		{
			log_warn("client %s tried reading and index that does not exist at %s", e->address, final_path);
			
			/* TODO send to error page and a callback to the error printer*/
			vert_pages_error(404, e);
		}
		
		
	}
	else
	{
		/* TODO use the filecache http sending function */
		vert_filecache_sendfile(final_path, &vert_pages_error, e);
	}
	
	
	vert_util_safe_free(final_path);
	vert_util_safe_free(temp_str);
	
	return SB_RES_OK;
}