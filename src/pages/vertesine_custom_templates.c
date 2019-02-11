#include "vertesine_custom_templates.h"
#include "../util.h"
#include "../cache.h"
#include "../templating.h"
#include "vertesine_util.h"

#include "tests.h"

#include <mkdio.h>
#include <sodium.h>

#include <time.h>
#include <string.h>
#include <ctype.h>

/* custom tag functions */

static char *custom_tag_banner(char *key, sb_Event *e)
{
	unsigned long size;
	
	/* TODO check if notifications.txt is empty */
	return vert_filecache_read("html/notification.htmpl", &size, VERT_CACHE_TEXT);
}

static char *custom_tag_notification(char *key, sb_Event *e)
{
	unsigned long size;
	
	return vert_filecache_read("html/notifications.txt", &size, VERT_CACHE_TEXT);
}

static char *custom_tag_navbar(char *key, sb_Event *e)
{
	unsigned long size;
	
	return vert_filecache_read("html/navbar.htmpl", &size, VERT_CACHE_TEXT);
}

static char *custom_tag_header(char *key, sb_Event *e)
{
	unsigned long size;
	
	return vert_filecache_read("html/header.htmpl", &size, VERT_CACHE_TEXT);
}

static char *custom_tag_footer(char *key, sb_Event *e)
{
	unsigned long size;
	
	return vert_filecache_read("html/footer.htmpl", &size, VERT_CACHE_TEXT);
}

static char *custom_tag_basecontainer_s(char *key, sb_Event *e)
{
	unsigned long size;
	
	return vert_filecache_read("html/basecontainer_begin.htmpl", &size, VERT_CACHE_TEXT);
}

static char *custom_tag_basecontainer_e(char *key, sb_Event *e)
{
	unsigned long size;
	
	return vert_filecache_read("html/basecontainer_end.htmpl", &size, VERT_CACHE_TEXT);
}

static char *custom_tag_seperator(char *key, sb_Event *e)
{
	return vert_util_strdup("<div class=\"seperator\"></div>");
}

static char *custom_tag_loginout(char *key, sb_Event *e)
{
	/* TODO check if the user is logged in */
	if(vert_custom_logged_in(e))
	{
		return vert_util_strdup("<li class=\"navbarli\"><a class=\"navbarli\" href=\"/logout\">LOGOUT</a></li>");
	}
	else
	{
		return vert_util_strdup("<li class=\"navbarli\"><a class=\"navbarli\" href=\"/login\">LOGIN</a></li>");
	}
}

static char *custom_tag_manage(char *key, sb_Event *e)
{
	char *return_str = NULL, *temp_path, *id_str = NULL, *permission_str = NULL; 
	uint64_t id;
	uint32_t permission_want = 0x80000000U, permission_want0 = 0x40000000U, permission_got; /* most signifigant bit */
	/* TODO check if the user is logged in */
	if((id = vert_custom_logged_in(e)))
	{
		vert_util_asprintf(&id_str, "%llu", id);
		
		permission_str = vert_custom_get_id_user_field(id_str, "permissions");
		
		permission_got = strtol(permission_str, NULL, 10);
		
		if(permission_want & permission_got || permission_want0 & permission_got) /* TODO allow moderators and admin 1.0's to view aswell */
		{
			return_str = vert_util_strdup("<li class=\"navbarli\"><a class=\"navbarli\" href=\"/manage\">MANAGEMENT PANEL</a></li>");
		}
		else
			return_str = vert_util_strdup("");
		
		
		vert_util_safe_free(id_str);
		vert_util_safe_free(permission_str);
	}
	else
		return_str = vert_util_strdup("");
	
	return return_str;
}

static char *custom_tag_userdispnav(char *key, sb_Event *e)
{
	/* TODO check if the user is logged in */
	char *return_str = NULL, *displayn, *usern, *pfploc, *id_str = NULL;
	uint64_t id;
	
	if((id = vert_custom_logged_in(e)))
	{
		/* get all of the data */
		vert_util_asprintf(&id_str, "%llu", id);
		
		usern = vert_custom_get_id_user_field(id_str, "username");
		pfploc = vert_custom_get_id_user_field(id_str, "pfp");
		displayn = vert_custom_get_id_user_field(id_str, "displayname");
		
		vert_util_asprintf(&return_str, "<li class=\"navbarli\" style=\"float: right;\"><a class=\"navbarli\" href=\"/users/%s\"><div style=\"usernavbar\"><img src=\"%s\" alt=\"usericon\" style=\"width:14px;height:14px;margin:0px;margin-right:5px;border-style:solid;border-width:1px;float:left;\">%s</div></a></li>", usern, pfploc, displayn);
		
		vert_util_safe_free(id_str);
		vert_util_safe_free(displayn);
		vert_util_safe_free(usern);
		vert_util_safe_free(pfploc);
		
		return return_str;
	}
	else
	{
		return vert_util_strdup("");
	}
}

static char *custom_tag_defpage(char *key, sb_Event *e)
{
	unsigned long size;
	
	return vert_filecache_read("html/default_page.htmpl", &size, VERT_CACHE_TEXT);
}

static char *custom_tag_md_editor(char *key, sb_Event *e)
{
	unsigned long size;
	char *editor = NULL, *temp_path, *markdown = NULL, *markdown_path = NULL, buffer[2048 * 1024], *id_str = NULL, *permission_str = NULL; /* 2mb max buffer for remote updating */
	uint64_t id;
	uint32_t permission_want = 0x80000000U, permission_got; /* most signifigant bit */
	
	
	editor = vert_util_strdup("");
	
	/* verify that the user has the ability to edit index markdown */
	if((id = vert_custom_logged_in(e)))
	{
		vert_util_asprintf(&id_str, "%llu", id);
		
		permission_str = vert_custom_get_id_user_field(id_str, "permissions");
		
		permission_got = strtol(permission_str, NULL, 10);
		
		if(permission_want & permission_got)
		{
			/* the user can edit the markup on the page */
			temp_path = vert_util_get_page_dir(e->path);
			
			vert_util_asprintf(&markdown_path, "%sindex.md", temp_path);
			
			/* TODO check if post and write new file to disk */
			if(strcmp(e->method, "POST") == 0)
			{
				log_warn("%s is being updated remotely", e->path);
				sb_get_var(e->stream, "data", buffer, sizeof(buffer));
				
				if(strlen(buffer))
					vert_util_write_file(markdown_path, buffer, strlen(buffer));
			}
			
			markdown = vert_filecache_read(markdown_path, &size, VERT_CACHE_TEXT);
			/* NOTE ============================ DO NOT USE MULTIPART FOR THIS */
			vert_util_safe_free(editor);
			editor = NULL;
			vert_util_asprintf(&editor, "<form action=\"%s\" method=\"post\">Edit page markup:<br><br><textarea name=\"data\" cols=\"130\" rows=\"30\">%s</textarea><br><input type=\"submit\" value=\"UPDATE\"></form>", e->path, markdown);
			
			vert_util_safe_free(temp_path);
			vert_util_safe_free(markdown_path);
			vert_util_safe_free(markdown);
		}
		
		
		vert_util_safe_free(id_str);
		vert_util_safe_free(permission_str);
	}
	
	
	
	
	return editor;
}

static char *custom_tag_markdown_content(char *key, sb_Event *e)
{
	/* TODO render markdown from the path/index.md or create the default if it isnt there */
	unsigned long size, output_size = 0;
	char *output = NULL, *path_to_md = NULL, *temp_ptr = NULL, temp_char;
	char *generate_md = "\
#markdown template\n\
* hello";
	uint8_t *md_file = NULL;
	MMIOT *marku = NULL;
	
	/* TODO make a universal path determiner */
	
	if(strstr(e->path, ".htmpl")) /* the path ends with the index selected */
	{
		if(!(temp_ptr = strrchr(e->path, '/')))
		{
			log_error("path not valid");
			return NULL;
		}
		temp_char = *temp_ptr;
		*temp_ptr = 0x00;
		
		vert_util_asprintf(&path_to_md, "html%s/index.md", e->path);
		
		*temp_ptr = temp_char;
	}
	else if(e->path[strlen(e->path) - 1] == '/') /* path ends with a trailing forward slash */
	{
		vert_util_asprintf(&path_to_md, "html%sindex.md", e->path);
	}
	else /* path ends without a trailing forward slash */
	{
		vert_util_asprintf(&path_to_md, "html%s/index.md", e->path);
	}
	
	
	if(!(md_file = vert_filecache_read(path_to_md, &size, VERT_CACHE_TEXT))) /* create a default index.md in the directory here because one does not exist then parse*/
	{
		log_info("generating markdown file at %s", path_to_md);
		vert_util_write_file(path_to_md, generate_md, strlen(generate_md));
		
		/* md_file = vert_util_strdup(generate_md); */
		md_file = vert_filecache_read(path_to_md, &size, VERT_CACHE_TEXT);
	}
	
	if((marku = mkd_string(md_file, size, 0)))
	{
		log_trace("parsing markdown");
		
		mkd_compile(marku, MKD_GITHUBTAGS | MKD_AUTOLINK | MKD_DLEXTRA | MKD_FENCEDCODE | MKD_LATEX | MKD_TOC);
		
		output_size = mkd_document(marku, &output);
		
		if(!output)
		{
			log_error("markdown html rendering error");
			
			output = vert_util_strdup("markdown render error");
		}
		else
		{
			output = vert_util_strdup(output); /* cleaning up after discount makes a double free error when returning */
		}

		mkd_cleanup(marku);
	}
	else
	{
		log_error("markdown load error");
		output = vert_util_strdup("markdown load error");
	}
	
	vert_util_safe_free(path_to_md);
	vert_util_safe_free(md_file);
	
	return output;
}

static char *custom_tag_year(char *key, sb_Event *e)
{
	time_t t = time(NULL);
	struct tm tms = *localtime(&t);
	char *time_str = NULL;
	
	vert_util_asprintf(&time_str, "%d", tms.tm_year + 1900);
	
	return time_str;
}

static char *custom_tag_mb_editor_container(char *key, sb_Event *e)
{
	unsigned long size;
	char *reply_str = NULL, *id_str = NULL, *permission_str = NULL;
	uint64_t id;
	uint32_t permission_want = 0x80000000U, permission_got; /* most signifigant bit */
	
	
	reply_str = vert_util_strdup("");
	
	/* verify that the user has the ability to edit index markdown */
	if((id = vert_custom_logged_in(e)))
	{
		vert_util_asprintf(&id_str, "%llu", id);
		
		permission_str = vert_custom_get_id_user_field(id_str, "permissions");
		
		permission_got = strtol(permission_str, NULL, 10);
		
		if(permission_want & permission_got)
		{
			vert_util_safe_free(reply_str);
			
			reply_str = vert_filecache_read("html/markup_editor.htmpl", &size, VERT_CACHE_TEXT);
		}
		
		
		vert_util_safe_free(id_str);
		vert_util_safe_free(permission_str);
	}
	
	
	return reply_str;
}

static char *custom_tag_redis_settings(char *key, sb_Event *e)
{
	char *final_str = NULL, *tok = NULL, *dup_reply = NULL, *temp_str;
	int i;
	redisReply *reply;
	
	reply = redisCommand(vert_redis_ctx,"INFO");
	
	vert_util_asprintf(&final_str, "<table><tr><th>qualities</th><th>value</th></tr>\n");
	
	if(reply)
	{
		dup_reply = vert_util_strdup(reply->str);
		
		tok = strtok(dup_reply, "\r\n");
		
		while(tok)
		{
			if((temp_str = strchr(tok, ':')))
			{
				*temp_str = 0x00;
				temp_str++;
			}
			else
				temp_str = tok;
			
			vert_util_asprintf(&final_str, "%s<tr><td>%s</td><td>%s</td></tr>\n", final_str, tok, temp_str);
			
			tok = strtok(NULL, "\r\n");
		}
		
		vert_util_safe_free(dup_reply);
		freeReplyObject(reply);
	}
	
	
	vert_util_asprintf(&final_str, "%s</table>\n", final_str);
	
	
	
	return final_str;
}

static char *custom_tag_register(char *key, sb_Event *e)
{
	int i;
	uint64_t id;
	redisReply *reply;
	char *return_str = NULL, *temp_str = NULL, username_buffer[44], email_buffer[257], password_buffer[102], password_v_buffer[102], hash[crypto_pwhash_STRBYTES], ip_buffer[22];
	
	
	/* TODO check if POSTing */
	if(strcmp(e->method, "POST") == 0)
	{
		/* check length of everything and verify password matches */
		sb_get_var(e->stream, "user", username_buffer, sizeof(username_buffer));
		
		if(strlen(username_buffer) < 1 || strlen(username_buffer) > 42)
		{
			return vert_util_strdup("<font color=\"red\">username less than 1 character or more than 42</font>");
		}
		
		for(i = 0; i < strlen(username_buffer); i++)
		{
			if(username_buffer[i] == ' ')
			{
				return vert_util_strdup("<font color=\"red\">username contains space</font>");
			}
		}
		
		/* check validity of email */
		
		sb_get_var(e->stream, "email", email_buffer, sizeof(email_buffer));
		
		if(strlen(email_buffer) < 3 || strlen(email_buffer) > 255)
		{
			return vert_util_strdup("<font color=\"red\">email less than 3 characters or more than 255</font>");
		}
		
		if(!strstr(email_buffer, ".") || !strstr(email_buffer, "@"))
		{
			return vert_util_strdup("<font color=\"red\">not valid email format</font>");
		}
		
		for(i = 0; i < strlen(email_buffer); i++)
		{
			if(email_buffer[i] == ' ')
			{
				return vert_util_strdup("<font color=\"red\">email contains space</font>");
			}
		}
		
		/* do password checking */
		
		sb_get_var(e->stream, "pass", password_buffer, sizeof(password_buffer));
		
		sb_get_var(e->stream, "passv", password_v_buffer, sizeof(password_v_buffer));
		
		if(strlen(password_buffer) < 5 || strlen(password_buffer) > 100 || strlen(password_v_buffer) < 5 || strlen(password_v_buffer) > 100)
		{
			return vert_util_strdup("<font color=\"red\">password shorter than 5 characters or greater than 100</font>");
		}
		
		if(strcmp(password_buffer, password_v_buffer) != 0)
		{
			return vert_util_strdup("<font color=\"red\">passwords do not match</font>");
		}
		
		for(i = 0; i < strlen(password_buffer); i++)
		{
			if(password_buffer[i] == ' ' || password_v_buffer[i] == ' ')
			{
				return vert_util_strdup("<font color=\"red\">password contains space</font>");
			}
		}
		
		/* check if the username is alphanumeric */
		
		/* set to all lowercase */
		
		for(i = 0; i < strlen(username_buffer); i++)
		{
			if(!isalnum(username_buffer[i]))
			{
				return vert_util_strdup("<font color=\"red\">username not alphanumeric</font>");
			}
		}
		
		for(i = 0; i < strlen(username_buffer); i++)
		{
			if(isalpha(username_buffer[i]))
			{
				username_buffer[i] = tolower(username_buffer[i]);
			}
		}
		
		
		
		/* check if the user already exists */
		reply = redisCommand(vert_redis_ctx, "LRANGE vertesine:variable:users 0 -1");
		if(reply)
		{
			if(reply->type == REDIS_REPLY_ARRAY)
				for(i = 0; i < reply->elements; i++)
				{
					/* data is stored as "id:username" */
					temp_str = strchr(reply->element[i]->str, ':');
					temp_str++;
					if(strcmp(temp_str, username_buffer) == 0)
					{
						freeReplyObject(reply);
						return vert_util_strdup("<font color=\"red\">user already exists</font>");
					}
					log_trace("looping through users %d %s", i, reply->element[i]->str);
				}
			
			freeReplyObject(reply);
		}
		
		
		/* TODO check if email already in use */
		
		log_info("creating user %s", username_buffer);
		
		/* if all tests pass, add to the list and create a hash */
		
		id = snowflake_id();
		
		reply = redisCommand(vert_redis_ctx, "LPUSH vertesine:variable:users %lu:%s", id, username_buffer);
		if(reply)
		{
			if(reply->type == REDIS_REPLY_ERROR)
			{
				freeReplyObject(reply);
				log_fatal("DB USER LIST ADDITION ERROR ===========================");
				return vert_util_strdup("<font color=\"red\">CRITICAL ERROR 0x001. <i>please</i> email me at zeroxthreef@gmail.com if you encounter this and please copy in the information you entered so I can help. Thanks!</font>");
			}
			freeReplyObject(reply);
		}
		
		/* create the hash */
		
		/* CREATE USER STUFF HERE */
		crypto_pwhash_str(hash, password_buffer, strlen(password_buffer), crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE);
		
		/* set the default user icon */
		switch(rand()%4)
		{
			case 0:
				temp_str = "/default_usericons/usericon.png";
			break;
			case 1:
				temp_str = "/default_usericons/usericon1.png";
			break;
			case 2:
				temp_str = "/default_usericons/usericon2.png";
			break;
			case 3:
				temp_str = "/default_usericons/usericon3.png";
			break;
		};
		
		
		
		reply = redisCommand(vert_redis_ctx, "HMSET vertesine:user:%lu id %lu version %d username %s displayname %s email %s permissions %lu phash %s pfp %s jointime %lu verified 0 deleted 0 reg_ip %s bio %s twitter %s channel %s website %s", id, id, VERT_USER_VERSION, username_buffer, username_buffer, email_buffer, 0UL, hash, temp_str, time(NULL), (sb_get_header(e->stream, "X-Real-IP", ip_buffer, sizeof(ip_buffer)) == SB_ESUCCESS) ? ip_buffer : e->address, "", "", "", "");
		if(reply)
		{
			if(reply->type == REDIS_REPLY_ERROR)
			{
				freeReplyObject(reply);
				log_fatal("DB USER KEY ADDITION ERROR ===========================");
				return vert_util_strdup("<font color=\"red\">CRITICAL ERROR 0x002. <i>please</i> email me at zeroxthreef@gmail.com if you encounter this and please copy in the information you entered so I can help. Thanks!</font>");
			}
			freeReplyObject(reply);
		}
		
		return vert_util_strdup("<font color=\"green\">creating user</font>");
	}
	
	return vert_util_strdup("");
}

static char *custom_tag_login(char *key, sb_Event *e)
{
	int i;
	uint64_t id;
	redisReply *reply;
	char *return_str = NULL, *temp_str = NULL, *temp1_str = NULL, *temp2_str = NULL, *token = NULL,  username_buffer[44], password_buffer[102], hash[crypto_pwhash_STRBYTES];
	
	
	/* TODO check if POSTing */
	if(strcmp(e->method, "POST") == 0)
	{
		sb_get_var(e->stream, "user", username_buffer, sizeof(username_buffer));
		
		if(strlen(username_buffer) < 1 || strlen(username_buffer) > 42)
		{
			return vert_util_strdup("<font color=\"red\">username less than 1 character or more than 42</font>");
		}
		
		sb_get_var(e->stream, "pass", password_buffer, sizeof(password_buffer));
		
		if(strlen(password_buffer) < 5 || strlen(password_buffer) > 100)
		{
			return vert_util_strdup("<font color=\"red\">password shorter than 5 characters or greater than 100</font>");
		}
		
		for(i = 0; i < strlen(username_buffer); i++)
		{
			if(isalpha(username_buffer[i]))
			{
				username_buffer[i] = tolower(username_buffer[i]);
			}
		}
		
		/* test if user exists, then get the id to verify the password hash*/
		
		reply = redisCommand(vert_redis_ctx, "LRANGE vertesine:variable:users 0 -1");
		if(reply)
		{
			if(reply->type == REDIS_REPLY_ARRAY)
				for(i = 0; i < reply->elements; i++)
				{
					/* data is stored as "id:username" */
					temp_str = strchr(reply->element[i]->str, ':');
					temp_str++;
					if(strcmp(temp_str, username_buffer) == 0)
					{
						/* found user */
						temp_str--;
						*temp_str = 0x00;
						temp_str = vert_util_strdup(reply->element[i]->str);
						
						freeReplyObject(reply);
						
						
						reply = redisCommand(vert_redis_ctx, "HGET vertesine:user:%s phash", temp_str);
						
						
						if(reply)
						{
							if(reply->type != REDIS_REPLY_ERROR)
							{
								
								if(crypto_pwhash_str_verify(reply->str, password_buffer, strlen(password_buffer)) != 0)
								{
									freeReplyObject(reply);
									log_warn("somebody tried to log in unsuccessfully to the account %s", username_buffer);
									vert_util_safe_free(temp_str);
									
									return vert_util_strdup("<font color=\"red\">incorrect password</font>");
								}
							}
							freeReplyObject(reply);
						}
						
						/* create a new user token */
						
						/* generate random string */
						temp1_str = vert_custom_random_string(*map_get(&vert_settings, "token_length"));
						
						/* tokens are stored as "id:tokenstr" */
						
						vert_util_asprintf(&token, "%s:%s", temp_str, temp1_str);
						
						
						vert_util_safe_free(temp1_str);
						
						reply = redisCommand(vert_redis_ctx, "SET vertesine:user_token:%s %s", temp_str, token);
						
						if(reply)
						{
							if(reply->type == REDIS_REPLY_ERROR)
							{
								
								freeReplyObject(reply);
								vert_util_safe_free(temp_str);
								vert_util_safe_free(token);
								
								return vert_util_strdup("<font color=\"red\">unable to create login token</font>");
							}
							freeReplyObject(reply);
						}
						
						
						/* set the token TTL */
						
						reply = redisCommand(vert_redis_ctx, "EXPIRE vertesine:user_token:%s %d", temp_str, *map_get(&vert_settings, "token_ttl"));
						
						if(reply)
						{
							if(reply->type == REDIS_REPLY_ERROR)
							{
								
								freeReplyObject(reply);
								vert_util_safe_free(temp_str);
								vert_util_safe_free(token);
								
								return vert_util_strdup("<font color=\"red\">unable to create login token stage 2</font>");
							}
							freeReplyObject(reply);
						}
						
						/* set the login cookie */
						
						log_trace("setting login cookie");
						
						temp2_str = vert_custom_get_http_date(time(NULL) + *map_get(&vert_settings, "token_ttl"));
						
						vert_util_asprintf(&token, "token=%s; HttpOnly; Expires=%s", token, temp2_str);
						
						vert_custom_set_get_cookie(NULL, token, 0);
						
						
						vert_util_safe_free(temp_str);
						vert_util_safe_free(temp2_str);
						vert_util_safe_free(token);
						
						return vert_util_strdup("<font color=\"green\">logging in</font>");
					}
					log_trace("looping through users %d %s", i, reply->element[i]->str);
				}
			
			freeReplyObject(reply);
		}
		
		return vert_util_strdup("<font color=\"red\">user does not exist</font>");
	}
	
	return vert_util_strdup("");
}

static char *custom_tag_logout(char *key, sb_Event *e)
{
	char *cookie = NULL, *cookie_data = NULL;
	
	if(vert_custom_logged_in(e))
	{
		cookie_data = vert_custom_get_http_date(time(NULL) - 100000); /* should be sufficiently in the past */
		
		vert_util_asprintf(&cookie, "token=%s; HttpOnly; Expires=%s", "chocolatechip", cookie_data);
		
		vert_custom_set_get_cookie(NULL, cookie, 0);
		
		vert_util_safe_free(cookie);
		vert_util_safe_free(cookie_data);
		
		return vert_util_strdup("eating cookies...");
	}
	
	
	
	return vert_util_strdup("eating cookies... wait a second... you aren't even logged in :(");
}

static char *custom_tag_title_real(char *key, sb_Event *e)
{
	/* TODO make this follow slashes as tokens for spaces */
	return vert_util_strdup("Vertesine");
}

static char *custom_tag_page_number_nav(char *key, sb_Event *e)
{
	unsigned long size;
	
	return vert_filecache_read("html/navigator.htmpl", &size, VERT_CACHE_TEXT);
}

static char *custom_tag_userpage_displayname(char *key, sb_Event *e)
{
	char *username = NULL, *temp_str = NULL;
	
	if((username = (strstr(e->path, "users/")) + strlen("users/")))
	{
		return vert_custom_get_username_user_field(username, "displayname");
	}
	
	return vert_util_strdup("ERROR");
}

static char *custom_tag_userpage_permissions(char *key, sb_Event *e)
{
	char *username = NULL, *temp_str = NULL;
	
	if((username = (strstr(e->path, "users/")) + strlen("users/")))
	{
		return vert_custom_get_username_user_field(username, "permissions");
	}
	
	return vert_util_strdup("ERROR");
}

static char *custom_tag_userpage_since(char *key, sb_Event *e)
{
	char *username = NULL, *temp_str = NULL, *temp0_str = NULL;
	
	if((username = (strstr(e->path, "users/")) + strlen("users/")))
	{
		temp0_str = vert_custom_get_username_user_field(username, "jointime");
		
		temp_str = vert_custom_get_http_date((time_t)strtoll(temp0_str, NULL, 10));
		
		vert_util_safe_free(temp0_str);
		
		return temp_str;
	}
	
	return vert_util_strdup("ERROR");
}

static char *custom_tag_userpage_pfp(char *key, sb_Event *e)
{
	char *username = NULL, *temp_str = NULL;
	
	if((username = (strstr(e->path, "users/")) + strlen("users/")))
	{
		return vert_custom_get_username_user_field(username, "pfp");
	}
	
	return vert_util_strdup("ERROR");
}

static char *custom_tag_userpage_info(char *key, sb_Event *e)
{
	char *username = NULL, *temp_str = NULL;
	unsigned long size, count = 0;
	
	if((username = (strstr(e->path, "users/")) + strlen("users/")))
	{
		if((temp_str = vert_custom_get_username_user_field(username, "bio")))
		{
			if(strlen(temp_str) > 0)
				count++;
			vert_util_safe_free(temp_str);
		}
		
		if((temp_str = vert_custom_get_username_user_field(username, "twitter")))
		{
			if(strlen(temp_str) > 0)
				count++;
			vert_util_safe_free(temp_str);
		}
		
		if((temp_str = vert_custom_get_username_user_field(username, "website")))
		{
			if(strlen(temp_str) > 0)
				count++;
			vert_util_safe_free(temp_str);
		}
		
		if((temp_str = vert_custom_get_username_user_field(username, "channel")))
		{
			if(strlen(temp_str) > 0)
				count++;
			vert_util_safe_free(temp_str);
		}
		
		if(count)
			return vert_filecache_read("html/user_info.htmpl", &size, VERT_CACHE_TEXT);
		else
			return vert_util_strdup("<center>no data provided</center>");
	}
	
	return vert_util_strdup("ERROR");
}

static char *custom_tag_userpage_edit_button(char *key, sb_Event *e)
{
	char *username = NULL, *temp_str = NULL, *temp0_str = NULL;
	uint64_t id;
	
	if((username = (strstr(e->path, "users/")) + strlen("users/")))
	{
		if((id = vert_custom_logged_in(e)))
		{
			temp_str = vert_custom_get_username_user_field(username, "id");
			
			if(id == strtoll(temp_str, NULL, 10))
			{
				vert_util_safe_free(temp_str);
				temp_str = NULL;
				
				temp0_str = vert_custom_get_username_user_field(username, "username");
				
				vert_util_asprintf(&temp_str, "<input type=\"button\" onclick=\"location.href='/users/edit/%s'\" value=\"EDIT\" />", temp0_str);
				vert_util_safe_free(temp0_str);
				
				return temp_str;
			}
			else
			{
				vert_util_safe_free(temp_str);
				return vert_util_strdup("");
			}
		}
		else
			return vert_util_strdup("");
	}
	
	return vert_util_strdup("ERROR");
}

static char *custom_tag_userpage_bio(char *key, sb_Event *e)
{
	char *username = NULL, *temp_str = NULL, *temp0_str = NULL, *markdown = NULL;
	MMIOT *marku = NULL;
	
	if((username = (strstr(e->path, "users/")) + strlen("users/")))
	{
		temp0_str = vert_custom_get_username_user_field(username, "bio");
		
		if(strlen(temp0_str))
		{
			if((marku = mkd_string(temp0_str, strlen(temp0_str), 0)))
			{
				
				mkd_compile(marku,
				((0b00000001000000000000000000000000 & vert_custom_get_username_user_permissions(username)) ? 0 : MKD_NOHTML));
				
				mkd_document(marku, &markdown);
				
				if(!markdown)
				{
					log_error("markdown html rendering error in user bio %s", username);
					
					markdown = vert_util_strdup("markdown render error. please email me if you see this");
				}
				else
				{
					vert_util_asprintf(&temp_str, "Bio:<div class=\"different_content3\">%s</div>", markdown);
				}
				
				mkd_cleanup(marku);
				/* NOTE no need to free markdown because its already cleaned up */
			}
		}
		else
			temp_str = vert_util_strdup("");
		
		vert_util_safe_free(temp0_str);
		
		return temp_str;
	}
	
	return vert_util_strdup("ERROR");
}

static char *custom_tag_userpage_website(char *key, sb_Event *e)
{
	char *username = NULL, *temp_str = NULL, *temp0_str = NULL;
	
	if((username = (strstr(e->path, "users/")) + strlen("users/")))
	{
		temp0_str = vert_custom_get_username_user_field(username, "website");
		
		if(strlen(temp0_str))
		{
			vert_util_asprintf(&temp_str, "Website:<br><a href=\"%s\">%s</a>", temp0_str, temp0_str);
		}
		else
			temp_str = vert_util_strdup("");
		
		vert_util_safe_free(temp0_str);
		
		return temp_str;
	}
	
	return vert_util_strdup("ERROR");
}

static char *custom_tag_userpage_channel(char *key, sb_Event *e)
{
	char *username = NULL, *temp_str = NULL, *temp0_str = NULL;
	
	if((username = (strstr(e->path, "users/")) + strlen("users/")))
	{
		temp0_str = vert_custom_get_username_user_field(username, "channel");
		
		if(strlen(temp0_str))
		{
			vert_util_asprintf(&temp_str, "Youtube Channel:<br><a href=\"https://youtube.com/channel/%s\">Channel Link</a>", temp0_str);
		}
		else
			temp_str = vert_util_strdup("");
		
		vert_util_safe_free(temp0_str);
		
		return temp_str;
	}
	
	return vert_util_strdup("ERROR");
}

static char *custom_tag_userpage_twitter(char *key, sb_Event *e)
{
	char *username = NULL, *temp_str = NULL, *temp0_str = NULL;
	
	if((username = (strstr(e->path, "users/")) + strlen("users/")))
	{
		temp0_str = vert_custom_get_username_user_field(username, "twitter");
		
		if(strlen(temp0_str))
		{
			vert_util_asprintf(&temp_str, "Twitter:<br><a href=\"https://twitter.com/%s\">@%s</a>", temp0_str, temp0_str);
		}
		else
			temp_str = vert_util_strdup("");
		
		vert_util_safe_free(temp0_str);
		
		return temp_str;
	}
	
	return vert_util_strdup("ERROR");
}

static char *custom_tag_edit_user_username(char *key, sb_Event *e)
{
	char *username = NULL;
	
	if((username = (strstr(e->path, "users/edit/")) + strlen("users/edit/")))
	{
		return vert_util_strdup(username);
	}
	
	return vert_util_strdup("ERROR");
}

static char *custom_tag_edit_user_pfp(char *key, sb_Event *e)
{
	char *username = NULL;
	
	if((username = (strstr(e->path, "users/edit/")) + strlen("users/edit/")))
	{
		return vert_custom_get_username_user_field(username, "pfp");
	}
	
	return vert_util_strdup("ERROR");
}

static char *custom_tag_edit_user_displayname(char *key, sb_Event *e)
{
	char *username = NULL;
	
	if((username = (strstr(e->path, "users/edit/")) + strlen("users/edit/")))
	{
		return vert_custom_get_username_user_field(username, "displayname");
	}
	
	return vert_util_strdup("ERROR");
}

static char *custom_tag_edit_user_bio(char *key, sb_Event *e)
{
	char *username = NULL;
	
	if((username = (strstr(e->path, "users/edit/")) + strlen("users/edit/")))
	{
		return vert_custom_get_username_user_field(username, "bio");
	}
	
	return vert_util_strdup("ERROR");
}

static char *custom_tag_edit_user_web(char *key, sb_Event *e)
{
	char *username = NULL;
	
	if((username = (strstr(e->path, "users/edit/")) + strlen("users/edit/")))
	{
		return vert_custom_get_username_user_field(username, "website");
	}
	
	return vert_util_strdup("ERROR");
}

static char *custom_tag_edit_user_twi(char *key, sb_Event *e)
{
	char *username = NULL;
	
	if((username = (strstr(e->path, "users/edit/")) + strlen("users/edit/")))
	{
		return vert_custom_get_username_user_field(username, "twitter");
	}
	
	return vert_util_strdup("ERROR");
}

static char *custom_tag_edit_user_channel(char *key, sb_Event *e)
{
	char *username = NULL;
	
	if((username = (strstr(e->path, "users/edit/")) + strlen("users/edit/")))
	{
		return vert_custom_get_username_user_field(username, "channel");
	}
	
	return vert_util_strdup("ERROR");
}

static char *custom_tag_edit_user_since(char *key, sb_Event *e)
{
	char *username = NULL, *temp_str = NULL, *temp0_str = NULL;
	
	if((username = (strstr(e->path, "users/edit/")) + strlen("users/edit/")))
	{
		temp0_str = vert_custom_get_username_user_field(username, "jointime");
		
		temp_str = vert_custom_get_http_date((time_t)strtoll(temp0_str, NULL, 10));
		
		vert_util_safe_free(temp0_str);
		
		return temp_str;
	}
	
	return vert_util_strdup("ERROR");
}

static char *custom_tag_edit_user_feedback(char *key, sb_Event *e)
{
	char *username = NULL, *temp_str = NULL, *temp0_str = NULL, *verify = NULL, *id_str = NULL, *return_str = NULL;
	char dispn_field[52], bio_field[4098], web_field[52], twi_field[52], channel_field[52];
	uint64_t id;
	uint8_t editing_text = 0, *pfp_data = NULL;
	size_t pfp_data_len = 0;
	
	if((username = (strstr(e->path, "users/edit/")) + strlen("users/edit/")))
	{
		if(strcmp(e->method, "POST") == 0)
		{
			verify = vert_custom_get_username_user_field(username, "id");
			
			if((id = vert_custom_logged_in(e)))
			{
				vert_util_asprintf(&id_str, "%llu", id);
				
				if(strcmp(id_str, verify) == 0)
				{
					/* user can edit this profile */
					
					/*verify if field edit or image edit */
					
					if(sb_get_var(e->stream, "user", dispn_field, sizeof(dispn_field)) != SB_ENOTFOUND)
						editing_text++;
					if(sb_get_var(e->stream, "bio", bio_field, sizeof(bio_field)) != SB_ENOTFOUND)
						editing_text++;
					if(sb_get_var(e->stream, "web", web_field, sizeof(web_field)) != SB_ENOTFOUND)
						editing_text++;
					if(sb_get_var(e->stream, "twi", twi_field, sizeof(twi_field)) != SB_ENOTFOUND)
						editing_text++;
					if(sb_get_var(e->stream, "you", channel_field, sizeof(channel_field)) != SB_ENOTFOUND)
						editing_text++;
					
					/* editing fields only */
					
					if(editing_text) /* no tab for the following if because reformatting sucks to do */
					if(strlen(dispn_field) > 1 && strlen(dispn_field) <= 50)
					{
						/* TODO check for permissions before chosing to filter html */
						temp0_str = vert_custom_get_username_user_field(username, "permissions");
						if(temp0_str)
						{
							
							temp_str = vert_util_strdup(dispn_field);
							
							if(strtoll(temp0_str, NULL, 10) & 0b00000010000000000000000000000000) /* determine if user can xss display name */
								vert_custom_filter_input(&temp_str, 0);
							else
								vert_custom_filter_input(&temp_str, 1);
								
							vert_util_safe_free(temp0_str);
							
							/* write the data to the db */
							
							if(vert_custom_write_hash_field(id, VERT_CUSTOM_OBJECT_USER, "displayname", temp_str))
								if(!return_str)
									return_str = vert_util_strdup("<font color=\"red\">DB error. If you see this, please email me at zeroxthreef@gmail.com</font>");
							
							vert_util_safe_free(temp_str);
							
						}
					}
					else
						if(!return_str)
							return_str = vert_util_strdup("<font color=\"red\">display name cant be longer than 50 characters or less than 1</font>");
					
					
					
					if(editing_text) /* no tab for the following if because reformatting sucks to do */
					if(strlen(bio_field) <= 4096)
					{
						temp_str = vert_util_strdup(bio_field);
						vert_custom_filter_tags(&temp_str); /* make sure users cant make the template manager do unwanted things */
						
						if(vert_custom_write_hash_field(id, VERT_CUSTOM_OBJECT_USER, "bio", temp_str))
							if(!return_str)
								return_str = vert_util_strdup("<font color=\"red\">DB error. If you see this, please email me at zeroxthreef@gmail.com</font>");
						
						vert_util_safe_free(temp_str);
					}
					else
						if(!return_str)
							return_str = vert_util_strdup("<font color=\"red\">bio cant be larger than 4096 characters</font>");
					
					
					
					if(editing_text) /* no tab for the following if because reformatting sucks to do */
					if(strlen(web_field) <= 50)
					{
						temp_str = vert_util_strdup(web_field);
						vert_custom_filter_input(&temp_str, 1);
						
						if(vert_custom_write_hash_field(id, VERT_CUSTOM_OBJECT_USER, "website", temp_str))
							if(!return_str)
								return_str = vert_util_strdup("<font color=\"red\">DB error. If you see this, please email me at zeroxthreef@gmail.com</font>");
								
						vert_util_safe_free(temp_str);
					}
					else
						if(!return_str)
							return_str = vert_util_strdup("<font color=\"red\">website URL cant be larger than 50 characters</font>");
					
					
					
					if(editing_text) /* no tab for the following if because reformatting sucks to do */
					if(strlen(twi_field) <= 50)
					{
						temp_str = vert_util_strdup(twi_field);
						vert_custom_filter_input(&temp_str, 1);
						
						if(vert_custom_write_hash_field(id, VERT_CUSTOM_OBJECT_USER, "twitter", temp_str))
							if(!return_str)
								return_str = vert_util_strdup("<font color=\"red\">DB error. If you see this, please email me at zeroxthreef@gmail.com</font>");
						
						vert_util_safe_free(temp_str);
					}
					else
						if(!return_str)
							return_str = vert_util_strdup("<font color=\"red\">twitter name cant be larger than 50 characters</font>");
					
					
					
					if(editing_text) /* no tab for the following if because reformatting sucks to do */
					if(strlen(channel_field) <= 50)
					{
						temp_str = vert_util_strdup(channel_field);
						vert_custom_filter_input(&temp_str, 1);
						
						if(vert_custom_write_hash_field(id, VERT_CUSTOM_OBJECT_USER, "channel", temp_str))
							if(!return_str)
								return_str = vert_util_strdup("<font color=\"red\">DB error. If you see this, please email me at zeroxthreef@gmail.com</font>");
						
						vert_util_safe_free(temp_str);
					}
					else
						if(!return_str)
							return_str = vert_util_strdup("<font color=\"red\">channel name cant be larger than 50 characters</font>");
					
					
					
					/* editing pfp only */
					
					if(!editing_text)
					{
						if(pfp_data = sb_get_multipart(e->stream, "pic", &pfp_data_len))
						{
							if(pfp_data_len > 4096 * 1024)
							{
								if(!return_str)
									return_str = vert_util_strdup("<font color=\"red\">the image was too large. Please keep files under 4mb</font>");
							}
							else
							{
								if(vert_custom_determine_extension(pfp_data, pfp_data_len))
								{
									if(vert_custom_manage_pfp(pfp_data, vert_custom_determine_extension(pfp_data, pfp_data_len), verify, pfp_data_len, &temp0_str))
									{
										if(!return_str)
											return_str = vert_util_strdup("<font color=\"red\">server could not handle the image uploaded</font>");
									}
									else
									{
										if(vert_custom_write_hash_field(id, VERT_CUSTOM_OBJECT_USER, "pfp", temp0_str))
										{
											if(!return_str)
												return_str = vert_util_strdup("<font color=\"red\">DB error. If you see this, please email me at zeroxthreef@gmail.com</font>");
										}
										else
										{
											log_info("user %s set their profile picture", username);
										}
										
									}
									//vert_util_safe_free(temp0_str);
								}
								else
									if(!return_str)
										return_str = vert_util_strdup("<font color=\"red\">please upload only png, jpg, gif, or bmp</font>");
								
							}
							
						}
						else
							if(!return_str)
								return_str = vert_util_strdup("<font color=\"red\">there was a problem with reading the image data. Please try again</font>");
					}
					
					if(!return_str)
						return_str = vert_util_strdup("<font color=\"green\">profile updated. Return to the view page for your profile to see the updates</font>");
				}
				else
					return_str = vert_util_strdup("<font color=\"red\">you cannot edit this profile</font>");
				
				
				vert_util_safe_free(id_str);
			}
			else
				return_str = vert_util_strdup("<font color=\"red\">you cannot edit this profile</font>");
			
			
			vert_util_safe_free(verify);
		}
		else
		{
			return_str = vert_util_strdup("");
		}
		
		
		return return_str;
	}
	
	return vert_util_strdup("ERROR");
}

static char *custom_tag_user_pagenums_calculate(char *key, sb_Event *e)
{
	char *fix_path = NULL;
	
	if(strstr(e->path, "/users"))
		return vert_util_strdup("[vert_custom_pagenums_user]");
	else if(strstr(e->path, "/entries"))
		return vert_util_strdup("[vert_custom_pagenums_entries]");
	else if(strstr(e->path, "/projects"))
		return vert_util_strdup("[vert_custom_pagenums_projects]");
	
	return vert_util_strdup("ERROR");
}

static char *custom_tag_user_pagenums_user(char *key, sb_Event *e)
{
	char *temp_str = NULL, number[64] = {0};
	size_t current = 1, max_elements;
	redisReply *reply;
	
	
	reply = redisCommand(vert_redis_ctx, "LLEN vertesine:variable:users");
	
	if(reply->type == REDIS_REPLY_INTEGER)
	{
		max_elements = reply->integer;
	}
	else
		max_elements = 1;
	
	freeReplyObject(reply);
	
	
	if(sb_get_var(e->stream, "p", number, sizeof(number)) != SB_ENOTFOUND)
	{
		current = strtoll(number, NULL, 10);
		
		if(current == 0)
			current = 1;
		else if(current * *map_get(&vert_settings, "elem_per_page") > max_elements + *map_get(&vert_settings, "elem_per_page"))
			current = 1; /*(max_elements + 1) / *map_get(&vert_settings, "elem_per_page"); */
	}
	
	
	temp_str = vert_custom_generate_pagenum_nav(e->path, current, max_elements);
	
	return temp_str;
}

static char *custom_tag_user_list(char *key, sb_Event *e)
{
	char *temp_str = NULL, *reply_str = NULL, *template_str = NULL, *temp0_str = NULL, number[64] = {0};
	unsigned long size;
	redisReply *reply;
	size_t i, current = 0, max_elements;
	
	
	if(!(template_str = vert_filecache_read("html/user_list_data.htmpl", &size, VERT_CACHE_TEXT)))
	{
		log_error("could not find the necessary template file for the user list template");
		return vert_util_strdup("template error in user list");
	}
	
	reply = redisCommand(vert_redis_ctx,"LRANGE vertesine:variable:users 0 -1");
	
	
	if(sb_get_var(e->stream, "p", number, sizeof(number)) != SB_ENOTFOUND)
	{
		current = (strtoll(number, NULL, 10) - 1) * *map_get(&vert_settings, "elem_per_page");
		
		max_elements = current + *map_get(&vert_settings, "elem_per_page");
		
		if(strtoll(number, NULL, 10) == 0) /* TODO fix 0 index problem  */
			current = 0;
		else if(current > reply->elements) /* just show the first page. Could just show a blank page but this will only happen if  someone is explicitly trying to go over the limit */
			current = 0;
		
		if(max_elements > reply->elements)
			max_elements = reply->elements;
		
	}
	else
	{
		current = 0;
		
		max_elements = current + *map_get(&vert_settings, "elem_per_page");
		
		if(max_elements > reply->elements)
			max_elements = reply->elements;
	}
	
	
	if(reply->type == REDIS_REPLY_ARRAY)
	{
		if(max_elements - current != 0)
			reply_str = vert_util_strdup(template_str);
		
		for(i = current; i < max_elements; i++)
		{
			vert_util_replace_buffer_all(&reply_str, "[vert_custom_usernamelink]", strchr(reply->element[i]->str, ':') + 1);
			
			temp_str = strchr(reply->element[i]->str, ':');
			*temp_str = 0x00;
			
			temp0_str = vert_custom_get_id_user_field(reply->element[i]->str, "pfp");
			vert_util_replace_buffer_all(&reply_str, "[vert_custom_userlist_icon]", temp0_str);
			vert_util_safe_free(temp0_str);
			
			temp0_str = vert_custom_get_id_user_field(reply->element[i]->str, "displayname");
			vert_util_replace_buffer_all(&reply_str, "[vert_custom_userlist_displayname]", temp0_str);
			vert_util_safe_free(temp0_str);
			
			vert_util_replace_buffer_all(&reply_str, "[vert_custom_userlist_permissions]", "display permissions TODO");
			
			vert_util_replace_buffer_all(&reply_str, "[vert_custom_userlist_id]", reply->element[i]->str);
			
			if(i + 1 < max_elements) /* used to be the elements */
				vert_util_asprintf(&reply_str, "%s%s", reply_str, template_str);
			else
				vert_util_asprintf(&reply_str, "%s", reply_str);
		}
	}
	
	freeReplyObject(reply);
	
	if(!reply_str)
		reply_str = vert_util_strdup(""); /* used to be ERROR */
		
	vert_util_safe_free(template_str);
	
	
	return reply_str;
}

static char *custom_tag_manage_notifications(char *key, sb_Event *e)
{
	char *return_str = NULL, *temp_path, *id_str = NULL, *permission_str = NULL; 
	uint64_t id;
	uint32_t permission_want = 0x80000000U, permission_want0 = 0x40000000U, permission_got; /* most signifigant bit */
	/* TODO check if the user is logged in */
	if((id = vert_custom_logged_in(e)))
	{
		vert_util_asprintf(&id_str, "%llu", id);
		
		permission_str = vert_custom_get_id_user_field(id_str, "permissions");
		
		permission_got = strtol(permission_str, NULL, 10);
		
		if(permission_want & permission_got || permission_want0 & permission_got) /* TODO allow moderators and admin 1.0's to view aswell */
		{
			return_str = vert_util_strdup("<font color=\"red\">proceed with caution</font>");
		}
		else
			return_str = vert_util_strdup("<font color=\"red\">access denied</font>");
		
		
		vert_util_safe_free(id_str);
		vert_util_safe_free(permission_str);
	}
	else
		return_str = vert_util_strdup("<font color=\"red\">access denied</font>");
	
	return return_str;
}

static char *custom_tag_manage_permissions(char *key, sb_Event *e)
{
	char *return_str = NULL, *temp_path, *id_str = NULL, *permission_str = NULL, *permission_file = NULL, *temp_str = NULL, *temp0_str = NULL, *update_str;
	char username[60], check[3], *setting_perm_str = NULL, *temp_perm_index = NULL;
	unsigned long size, i;
	uint64_t id;
	uint32_t permission_want = 0x80000000U, permission_want0 = 0x40000000U, permission_got; /* most signifigant bit */
	/* TODO check if the user is logged in */
	if((id = vert_custom_logged_in(e)))
	{
		vert_util_asprintf(&id_str, "%llu", id);
		
		permission_str = vert_custom_get_id_user_field(id_str, "permissions");
		
		permission_got = strtol(permission_str, NULL, 10);
		
		if(permission_want & permission_got || permission_want0 & permission_got) /* TODO allow moderators and admin 1.0's to view aswell */
		{
			
			username[0] = 0x00;
			
			if(sb_get_var(e->stream, "uperm", username, sizeof(username)) != SB_ENOTFOUND)
			{
				if(setting_perm_str = vert_custom_get_username_user_field(username, "permissions"))
				{
					
					permission_got = strtol(setting_perm_str, NULL, 10);
					
					if(sb_get_var(e->stream, "perm_box_set", check, sizeof(check)) != SB_ENOTFOUND)
					{
						update_str = "updating user...";
						/* actually update user */
						
						permission_got = 0;
						
						for(i = 0; i < 32; i++)
						{
							vert_util_asprintf(&temp_perm_index, "perm_box_%lu", i + 1);
							
							if(sb_get_var(e->stream, temp_perm_index, check, sizeof(check)) != SB_ENOTFOUND)
							{
								permission_got |= 1U << i;
							}
							
						}
						
						vert_util_asprintf(&temp_perm_index, "%lu", permission_got); /* reusing variables because its easy enough to remember here */
						
						temp_str = vert_custom_get_username_user_field(username, "id");
						
						vert_custom_write_hash_field(strtoll(temp_str, NULL, 10), VERT_CUSTOM_OBJECT_USER, "permissions", temp_perm_index);
						
						vert_util_safe_free(temp_perm_index);
						vert_util_safe_free(temp_str);
						temp_str = NULL;
					}
					else
					{
						update_str = "fetched user permissions";
					}
				}
				else
					update_str = "user does not exist";
				
			}
			else
				update_str = "";
			
			
			permission_file = vert_filecache_read("permissions.txt", &size, VERT_CACHE_TEXT);
			
			temp_str = strtok(permission_file, "|\n");
			
			vert_util_asprintf(&return_str, "<h1>edit user permissions</h1>\n<br><font color=\"cyan\">%s</font><br>\nDirections: UNCHECK the \"set\" checkbox to get the users current permisisons. Check the same checkbox \
to set the users permission AFTER getting it.<br><br>\n<form action=\"/manage\" method=\"post\">\nUSERNAME:<br><br>\n<input type=\"text\" name=\"uperm\" value=\"%s\"><br><br><table>", update_str, username);
			
			vert_util_asprintf(&return_str, "%s<tr><td>SET PERMISSION</td><td><input type=\"checkbox\" name=\"perm_box_set\" value=\"1\"></td></tr>", return_str);
			
			/* TODO tokenize the perms.txt */
			i = 0;
			while(temp_str)
			{
				temp0_str = strtok(NULL, "|\n");
				
				if(sb_get_var(e->stream, "uperm", username, sizeof(username)) != SB_ENOTFOUND && vert_custom_check_user_exists(username))
				{
					vert_util_asprintf(&return_str, "%s<tr><td>%s</td><td><input type=\"checkbox\" name=\"perm_box_%s\" value=\"1\"%s></td></tr>", return_str, temp0_str, temp_str, (permission_want & (permission_got << i)) ? " checked" : "");
				}
				else
				{
					vert_util_asprintf(&return_str, "%s<tr><td>%s</td><td><input type=\"checkbox\" name=\"perm_box_%s\" value=\"1\"></td></tr>", return_str, temp0_str, temp_str);
				}
				
				temp_str = strtok(NULL, "|\n");
				i++;
			}
			
			
			
			vert_util_asprintf(&return_str, "%s</table>\n<br><br>\n<input type=\"submit\" value=\"UPDATE USER PERMISSIONS\">\n</form>", return_str);
			
			vert_util_safe_free(permission_file);
			vert_util_safe_free(setting_perm_str);
		}
		else
			return_str = vert_util_strdup("");
		
		
		vert_util_safe_free(id_str);
		vert_util_safe_free(permission_str);
	}
	else
		return_str = vert_util_strdup("");
	
	return return_str;
}

static char *custom_tag_manage_deletion(char *key, sb_Event *e)
{
	char *return_str = NULL, *temp_path, *id_str = NULL, *permission_str = NULL, username[60], *temp_str = NULL, *temp0_str = NULL, *temp1_str = NULL; 
	uint64_t id;
	uint32_t permission_want = 0x80000000U, permission_got; /* most signifigant bit */
	redisReply *reply;
	
	username[0] = 0x00;
	
	/* TODO check if the user is logged in */
	if((id = vert_custom_logged_in(e)))
	{
		vert_util_asprintf(&id_str, "%llu", id);
		
		permission_str = vert_custom_get_id_user_field(id_str, "permissions");
		
		permission_got = strtol(permission_str, NULL, 10);
		
		if(permission_want & permission_got)
		{
			if(sb_get_var(e->stream, "deluser", username, sizeof(username)) != SB_ENOTFOUND)
			{
				/* test if the user exists */
				if(vert_custom_check_user_exists(username))
				{
					/* remove the user from the list and the user key */
					
					
					temp_str = vert_custom_get_username_user_field(username, "id");
					
					asprintf(&temp0_str, "%s:%s", temp_str, username);
					
					
					reply = redisCommand(vert_redis_ctx,"LREM vertesine:variable:users 0 %s", temp0_str);
					
					if(reply->integer == 1) /* user was removed from the list */
					{
						freeReplyObject(reply);
						reply = NULL;
						
						/* remove the key now */
						
						/* remove filesystem resources */
						id = strtoll(temp_str, NULL, 10);
						
						vert_custom_delete_usericon(temp_str);
						
						temp1_str = vert_custom_generate_db_key(id, VERT_CUSTOM_OBJECT_USER);
						
						reply = redisCommand(vert_redis_ctx,"DEL %s", temp1_str);
						
						if(reply->type != REDIS_REPLY_ERROR)
							vert_util_asprintf(&return_str, "<h1>delete user</h1>\n<br><font color=\"cyan\">deleted user %s</font><br>\nDirections: Set the username and click \"delete\". zip zap zoing, user deletus<br><br>\n<form action=\"/manage\" method=\"post\">\nUSERNAME:<br><br>\n<input type=\"text\" name=\"deluser\">\n<br><br>\n<input type=\"submit\" value=\"DELETE USER\">\n</form>", username);
						else
							vert_util_asprintf(&return_str, "<h1>delete user</h1>\n<br><font color=\"cyan\">user %s is in an errored state</font><br>\nDirections: Set the username and click \"delete\". zip zap zoing, user deletus<br><br>\n<form action=\"/manage\" method=\"post\">\nUSERNAME:<br><br>\n<input type=\"text\" name=\"deluser\">\n<br><br>\n<input type=\"submit\" value=\"DELETE USER\">\n</form>", username);
						
						vert_util_safe_free(temp1_str);
						
						/* I honestly dont care if the token gets deleted so i wont check */
						freeReplyObject(reply);
						reply = NULL;
						
						temp1_str = vert_custom_generate_db_key(id, VERT_CUSTOM_OBJECT_UTOKEN);
						
						reply = redisCommand(vert_redis_ctx,"DEL %s", temp1_str);
						
						
						vert_util_safe_free(temp1_str);
					}
					else
						vert_util_asprintf(&return_str, "<h1>delete user</h1>\n<br><font color=\"cyan\">user %s has an errored db state</font><br>\nDirections: Set the username and click \"delete\". zip zap zoing, user deletus<br><br>\n<form action=\"/manage\" method=\"post\">\nUSERNAME:<br><br>\n<input type=\"text\" name=\"deluser\">\n<br><br>\n<input type=\"submit\" value=\"DELETE USER\">\n</form>", username);
					
					if(reply)
						freeReplyObject(reply);
					
					
					vert_util_safe_free(temp_str);
					vert_util_safe_free(temp0_str);
				}
				else
					vert_util_asprintf(&return_str, "<h1>delete user</h1>\n<br><font color=\"cyan\">user %s does not exist</font><br>\nDirections: Set the username and click \"delete\". zip zap zoing, user deletus<br><br>\n<form action=\"/manage\" method=\"post\">\nUSERNAME:<br><br>\n<input type=\"text\" name=\"deluser\">\n<br><br>\n<input type=\"submit\" value=\"DELETE USER\">\n</form>", username);
			}
			else
			{
				vert_util_asprintf(&return_str, "<h1>delete user</h1>\n<br><font color=\"cyan\">%s</font><br>\nDirections: Set the username and click \"delete\". zip zap zoing, user deletus<br><br>\n<form action=\"/manage\" method=\"post\">\nUSERNAME:<br><br>\n<input type=\"text\" name=\"deluser\">\n<br><br>\n<input type=\"submit\" value=\"DELETE USER\">\n</form>", "");
			}
		}
		else
			return_str = vert_util_strdup("");
		
		
		vert_util_safe_free(id_str);
		vert_util_safe_free(permission_str);
	}
	else
		return_str = vert_util_strdup("");
	
	return return_str;
}

static char *custom_tag_create_entry(char *key, sb_Event *e)
{
	char *return_str = NULL, *temp_path, *id_str = NULL, *permission_str = NULL; 
	unsigned long size;
	uint64_t id;
	uint32_t permission_want = 0x80000000U, permission_want0 = 0x40000000U, permission_got; /* most signifigant bit */
	/* TODO check if the user is logged in */
	if((id = vert_custom_logged_in(e)))
	{
		vert_util_asprintf(&id_str, "%llu", id);
		
		permission_str = vert_custom_get_id_user_field(id_str, "permissions");
		
		permission_got = strtol(permission_str, NULL, 10);
		
		if(permission_want & permission_got) /* NOTE: maybe add the possibility for others to post, but thats not urgent */ /* || permission_want0 & permission_got) /* TODO allow moderators and admin 1.0's to view aswell */
		{
			return_str = vert_filecache_read("html/create_entry.htmpl", &size, VERT_CACHE_TEXT);
		}
		else
			return_str = vert_util_strdup("");
		
		
		vert_util_safe_free(id_str);
		vert_util_safe_free(permission_str);
	}
	else
		return_str = vert_util_strdup("");
	
	return return_str;
}

static char *custom_tag_entry_feedback(char *key, sb_Event *e)
{
	char *return_str = NULL, *temp_path, *id_str = NULL, *permission_str = NULL, *temp_str = NULL, entry_title[102]; 
	uint64_t id;
	redisReply *reply;
	size_t i;
	uint32_t permission_want = 0x80000000U, permission_want0 = 0x40000000U, permission_got; /* most signifigant bit */
	/* TODO check if the user is logged in */
	
	/* entry hashmap layout thoughts */
	
	/*
	 * vertesine:entry:{id}
	 * 
	 * id
	 * title
	 * subtitle?
	 * visibility to permissions
	 * body text
	 * publish date (0 if unpublished)
	 * last edit timestamp (0 if never)
	 * last edit commit message
	 */
	 
	 /* entry file list */
	 
	 /* vertesine:entry_filelist:{id} */
	 /* a list of filenames and stuff. NOTE: remember to add a FETCH zip and SET zip file control
		because zip files will represent data in the directory. Any files that arent hashed as the exact same will be replaced */
	
	/* add an icon.png to the dir if wanting a banner or something. Will come up with that later/save that feature for projects */
	
	
	
	if((id = vert_custom_logged_in(e)))
	{
		vert_util_asprintf(&id_str, "%llu", id);
		
		permission_str = vert_custom_get_id_user_field(id_str, "permissions");
		
		permission_got = strtol(permission_str, NULL, 10);
		
		if(permission_want & permission_got) /* same deal as the function above this one */ /* || permission_want0 & permission_got) /* TODO allow moderators and admin 1.0's to view aswell */
		{
			
			if(sb_get_var(e->stream, "post_title", entry_title, sizeof(entry_title)) != SB_ENOTFOUND)
			{
				/* make sure there are no other entries with this name */
				log_warn("entry creation attempt");
				
				reply = redisCommand(vert_redis_ctx, "LRANGE vertesine:variable:entries 0 -1");
				if(reply->type == REDIS_REPLY_ARRAY)
				{
					for(i = 0; i < reply->elements; i++)
					{
						/* data is stored as "id:title" */
						temp_str = strchr(reply->element[i]->str, ':');
						temp_str++;
						if(strcmp(temp_str, entry_title) == 0)
						{
							/* title collision */
							temp_str--;
							*temp_str = 0x00;
							
							if(!return_str)
								return_str = vert_util_strdup("<font color=\"red\">an entry with that title already exists</font>");
							
							break;
						}
					}
					
					freeReplyObject(reply);
					
				}
				
				if(!return_str) /* entry does not already exist */
				{
					/* create that entry with HMSET and add to the vertesine:variable:entries var */
					id = snowflake_id();
					
					reply = redisCommand(vert_redis_ctx, "LPUSH vertesine:variable:entries %llu:%s", id, entry_title);
					if(reply->type == REDIS_REPLY_ERROR)
					{
						log_error("could not create entry. REDIS list addition error");
						return_str = vert_util_strdup("<font color=\"red\">could not create entry. Please check the entries variable</font>");
					}
					
					freeReplyObject(reply);
					
					if(!return_str)
					{
						reply = redisCommand(vert_redis_ctx, "HMSET vertesine:entry:%llu id %llu title %s subtitle %s viewable_permissions %llu published %llu publishdate %llu editdate %llu editmessage %s", id, id, entry_title, "", 0, 0, 0, 0, "");
						if(reply->type == REDIS_REPLY_ERROR)
						{
							log_error("could not create entry. REDIS entry key set error. CLEAN THE ENTRIES LIST VARIABLE IMMEDIATELY AND REMOVE: %llu:%s", id, entry_title);
							return_str = vert_util_strdup("<font color=\"red\">could not create entry. The entries variable is in an errored state. Please clean manually and check the log for details</font>");
						}
						
						/* created an unpublished entry */
						
						return_str = vert_util_strdup("<font color=\"green\">created unpublished entry successfully</font>");
						
						freeReplyObject(reply);
					}
					
				}
			}
			else /* its a GET most likely */
				return_str = vert_util_strdup("");
			
			
		}
		else
			return_str = vert_util_strdup("<font color=\"red\">access denied. This message is impossible to see under normal circumstances</font>");
		
		
		vert_util_safe_free(id_str);
		vert_util_safe_free(permission_str);
	}
	else
		return_str = vert_util_strdup("<font color=\"red\">access denied. This message is impossible to see under normal circumstances</font>");
	
	return return_str;
}

static char *custom_tag_user_pagenums_entries(char *key, sb_Event *e)
{
	char *temp_str = NULL, number[64] = {0}, *temp_str1 = NULL;
	size_t current = 1, max_elements, i;
	uint32_t entry_permissions = 0;
	redisReply *reply;
	
	/* TODO check if an admin is browsing the page and display everything anyway. Show unpublished and per permission to other users aswell */
	
	/* permissionwise, the only time that the visibility is affected is when an entry is unpublished, user is set to
	BLOCKEDFROMREADINGPOSTS, or when there is no user logged in */
	
	
	reply = redisCommand(vert_redis_ctx, "LRANGE vertesine:variable:entries 0 -1");
	
	if(reply->type == REDIS_REPLY_ARRAY)
	{
		/* loop through all of the entries and count ones that the current user is allowed to see */
		for(i = 0; i < reply->elements; i++)
		{
			temp_str = vert_extract_list_elem(reply->element[i]->str, 0);
			
			temp_str1 = vert_custom_get_id_generic_field(temp_str, VERT_CUSTOM_OBJECT_ENTRY, "viewable_permissions");
			entry_permissions = strtoll(temp_str1, NULL, 10);
			vert_util_safe_free(temp_str1);
			
			/* TODO test if the user has permission to view this */
			
			vert_util_safe_free(temp_str);
		}
		
		max_elements = reply->integer;
	}
	else
		max_elements = 1;
	
	freeReplyObject(reply);
	
	
	if(sb_get_var(e->stream, "p", number, sizeof(number)) != SB_ENOTFOUND)
	{
		current = strtoll(number, NULL, 10);
		
		if(current == 0)
			current = 1;
		else if(current * *map_get(&vert_settings, "elem_per_page") > max_elements + *map_get(&vert_settings, "elem_per_page"))
			current = 1; /*(max_elements + 1) / *map_get(&vert_settings, "elem_per_page"); */
	}
	
	
	temp_str = vert_custom_generate_pagenum_nav(e->path, current, max_elements);
	
	return temp_str;
}

static char *custom_tag_entry_list(char *key, sb_Event *e)
{
	char *temp_str = NULL, *reply_str = NULL, *template_str = NULL, *temp0_str = NULL, number[64] = {0};
	unsigned long size;
	redisReply *reply;
	size_t i, current = 0, max_elements;
	
	
	if(!(template_str = vert_filecache_read("html/entry_list_data.htmpl", &size, VERT_CACHE_TEXT)))
	{
		log_error("could not find the necessary template file for the user list template");
		return vert_util_strdup("template error in user list");
	}
	
	reply = redisCommand(vert_redis_ctx,"LRANGE vertesine:variable:users 0 -1");
	
	
	if(sb_get_var(e->stream, "p", number, sizeof(number)) != SB_ENOTFOUND)
	{
		current = (strtoll(number, NULL, 10) - 1) * *map_get(&vert_settings, "elem_per_page");
		
		max_elements = current + *map_get(&vert_settings, "elem_per_page");
		
		if(strtoll(number, NULL, 10) == 0) /* TODO fix 0 index problem  */
			current = 0;
		else if(current > reply->elements) /* just show the first page. Could just show a blank page but this will only happen if  someone is explicitly trying to go over the limit */
			current = 0;
		
		if(max_elements > reply->elements)
			max_elements = reply->elements;
		
	}
	else
	{
		current = 0;
		
		max_elements = current + *map_get(&vert_settings, "elem_per_page");
		
		if(max_elements > reply->elements)
			max_elements = reply->elements;
	}
	
	
	if(reply->type == REDIS_REPLY_ARRAY)
	{
		if(max_elements - current != 0)
			reply_str = vert_util_strdup(template_str);
		
		for(i = current; i < max_elements; i++)
		{
			vert_util_replace_buffer_all(&reply_str, "[vert_custom_usernamelink]", strchr(reply->element[i]->str, ':') + 1);
			
			temp_str = strchr(reply->element[i]->str, ':');
			*temp_str = 0x00;
			
			temp0_str = vert_custom_get_id_user_field(reply->element[i]->str, "pfp");
			vert_util_replace_buffer_all(&reply_str, "[vert_custom_userlist_icon]", temp0_str);
			vert_util_safe_free(temp0_str);
			
			temp0_str = vert_custom_get_id_user_field(reply->element[i]->str, "displayname");
			vert_util_replace_buffer_all(&reply_str, "[vert_custom_userlist_displayname]", temp0_str);
			vert_util_safe_free(temp0_str);
			
			vert_util_replace_buffer_all(&reply_str, "[vert_custom_userlist_permissions]", "display permissions TODO");
			
			vert_util_replace_buffer_all(&reply_str, "[vert_custom_userlist_id]", reply->element[i]->str);
			
			if(i + 1 < max_elements) /* used to be the elements */
				vert_util_asprintf(&reply_str, "%s%s", reply_str, template_str);
			else
				vert_util_asprintf(&reply_str, "%s", reply_str);
		}
	}
	
	freeReplyObject(reply);
	
	if(!reply_str)
		reply_str = vert_util_strdup(""); /* used to be ERROR */
		
	vert_util_safe_free(template_str);
	
	
	return reply_str;
}

/* ==================================================================== */
/* ==================================================================== */
/* ==================================================================== */

void vert_custom_templ_add()
{
	/* vert_templ_add("[]", &custom_test_tag); */
	vert_templ_add("[vert_banner]", &custom_tag_banner);
	vert_templ_add("[vert_notification]", &custom_tag_notification);
	vert_templ_add("[vert_navbar]", &custom_tag_navbar);
	vert_templ_add("[vert_header]", &custom_tag_header);
	vert_templ_add("[vert_footer]", &custom_tag_footer);
	vert_templ_add("[vert_basecontainer_begin]", &custom_tag_basecontainer_s);
	vert_templ_add("[vert_basecontainer_end]", &custom_tag_basecontainer_e);
	vert_templ_add("[vert_seperator]", &custom_tag_seperator);
	vert_templ_add("[vert_year]", &custom_tag_year);
	vert_templ_add("[vert_loginlogout_nav]", &custom_tag_loginout);
	vert_templ_add("[vert_managementpanel_nav]", &custom_tag_manage);
	vert_templ_add("[vert_loginuser_nav]", &custom_tag_userdispnav);
	vert_templ_add("[vert_title]", &custom_tag_year); /* TODO fix this */
	vert_templ_add("[vert_title_real]", &custom_tag_title_real);
	vert_templ_add("[vert_default_page]", &custom_tag_defpage);
	vert_templ_add("[vert_page_content]", &custom_tag_markdown_content);
	vert_templ_add("[vert_page_markup_editor]", &custom_tag_mb_editor_container);
	vert_templ_add("[vert_page_markup_editor_text]", &custom_tag_md_editor);
	vert_templ_add("[vert_redis_status]", &custom_tag_redis_settings);
	vert_templ_add("[vert_register_feedback]", &custom_tag_register);
	vert_templ_add("[vert_login_feedback]", &custom_tag_login);
	vert_templ_add("[vert_logout]", &custom_tag_logout);
	vert_templ_add("[vert_page_num_nav]", &custom_tag_page_number_nav);
	vert_templ_add("[vert_current_user_displayname]", &custom_tag_userpage_displayname);
	vert_templ_add("[vert_current_user_permission_draw]", &custom_tag_userpage_permissions);
	vert_templ_add("[vert_current_user_since]", &custom_tag_userpage_since);
	vert_templ_add("[vert_current_user_pfp_path]", &custom_tag_userpage_pfp);
	vert_templ_add("[vert_current_user_information]", &custom_tag_userpage_info);
	vert_templ_add("[vert_current_user_edit_button]", &custom_tag_userpage_edit_button);
	vert_templ_add("[vert_current_user_bio]", &custom_tag_userpage_bio);
	vert_templ_add("[vert_current_user_website]", &custom_tag_userpage_website);
	vert_templ_add("[vert_current_user_youtube_channel]", &custom_tag_userpage_channel);
	vert_templ_add("[vert_current_user_twitter]", &custom_tag_userpage_twitter);
	vert_templ_add("[vert_edit_user_username]", &custom_tag_edit_user_username);
	vert_templ_add("[vert_edit_user_feedback]", &custom_tag_edit_user_feedback);
	vert_templ_add("[vert_edit_user_pfp]", &custom_tag_edit_user_pfp);
	vert_templ_add("[vert_edit_user_displayname]", &custom_tag_edit_user_displayname);
	vert_templ_add("[vert_edit_user_bio]", &custom_tag_edit_user_bio);
	vert_templ_add("[vert_edit_user_website]", &custom_tag_edit_user_web);
	vert_templ_add("[vert_edit_user_twitter]", &custom_tag_edit_user_twi);
	vert_templ_add("[vert_edit_user_channel]", &custom_tag_edit_user_channel);
	vert_templ_add("[vert_edit_user_since]", &custom_tag_edit_user_since);
	vert_templ_add("[vert_custom_pagenums_calculate]", &custom_tag_user_pagenums_calculate);
	vert_templ_add("[vert_custom_pagenums_user]", &custom_tag_user_pagenums_user);
	vert_templ_add("[vert_users]", &custom_tag_user_list);
	vert_templ_add("[vert_manage_notification]", &custom_tag_manage_notifications);
	vert_templ_add("[vert_manage_permissions]", &custom_tag_manage_permissions);
	vert_templ_add("[vert_manage_account_deletion]", &custom_tag_manage_deletion);
	vert_templ_add("[vert_entries_create]", &custom_tag_create_entry);
	vert_templ_add("[vert_create_entry_feedback]", &custom_tag_entry_feedback);
	vert_templ_add("[vert_entries]", &custom_tag_entry_list);
	vert_templ_add("[vert_custom_pagenums_entries]", &custom_tag_user_pagenums_entries);
	
	/* add the tests */
	log_info("adding the custom test tags");
	vert_add_tests();
}