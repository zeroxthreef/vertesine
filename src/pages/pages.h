#ifndef PAGES_H__
#define PAGES_H__

#include "../util.h"

void vert_pages_error(int error, sb_Event *e);

void vert_pages_init();

void vert_pages_destroy();

char *vert_pages_virtual_route(char *path);

int vert_pages_route(sb_Event *e);

#endif