#ifndef TESTS_H__
#define TESTS_H__

#include "vertesine_util.h"
#include "../util.h"

static char *custom_test_tag_perm(char *key, sb_Event *e)
{
	char *final_str = NULL;
	uint32_t test_perm = 0b00000010000000000000000000000001; /* 26, 1 */
	
	vert_util_asprintf(&final_str, "ok, so the one thing i guess looks like 0b00000010000000000000000000000000, and we'll test for the xss perm which returns %d", vert_contains_perm(test_perm, 26));
	
	return final_str;
}

void vert_add_tests()
{
	vert_templ_add("[vert_test_permission]", &custom_test_tag_perm);
}

#endif