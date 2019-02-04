#ifndef TEMPLATING_H__
#define TEMPLATING_H__

#include "util.h"

typedef char *(*vert_templ_callback_t)(char *key, sb_Event *e);

typedef map_t(vert_templ_callback_t) vert_templ_t;


vert_templ_t template_callbacks;


int vert_templ_init();

void vert_templ_destroy();

void vert_templ_add(char *key, vert_templ_callback_t callback);

void vert_templ_run(char **template_page_buffer, sb_Event *e);

#endif