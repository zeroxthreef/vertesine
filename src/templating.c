#include "templating.h"



int vert_templ_init()
{
	log_info("init templating system");
	
	map_init(&template_callbacks);
	
	return 0;
}

void vert_templ_destroy()
{
	log_info("destroying templating system");
	
	map_deinit(&template_callbacks);
	
}

void vert_templ_add(char *key, vert_templ_callback_t callback)
{
	log_info("add template \"%s\"", key);
	
	map_set(&template_callbacks, key, callback);
}

void vert_templ_run(char **template_page_buffer, sb_Event *e) /*pass it the template text data */
{
	char *key = NULL, *return_str = NULL;
	map_iter_t i = map_iter(&template_callbacks);
	vert_templ_callback_t cb;
	
	while((key = map_next(&template_callbacks, &i)) != NULL)
	{
		if(strstr(*template_page_buffer, key))
		{
			log_trace("calling %s", key);
			
			cb = *map_get(&template_callbacks, key);
		
			return_str = cb(key, e);

			/* modify the buffer */
			if(return_str)
			{
				vert_util_replace_buffer_all(template_page_buffer, key, return_str);
		
				vert_util_safe_free(return_str);
			
				/* run again in case more tags were added */
				vert_templ_run(template_page_buffer, e); 
			}
			else
			{
				log_error("tag error. Faulty return string for tag %s", key);
				
				vert_util_replace_buffer_all(template_page_buffer, key, "(tag error. critical server problem)");
			}
			
		}
		
	}
}