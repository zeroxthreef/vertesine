#ifndef HOME_H__
#define HOME_H__

short init();

int replace_normal_vertesine_elements(struct kreq *req, char **buffer);

void respond_debug(struct kreq *req);

void respond_confirmlogout(struct kreq *req);

void respond_home(struct kreq *req);

void respond_about(struct kreq *req);

void respond_manage(struct kreq *req);

void respond_user(struct kreq *req);

void respond_login(struct kreq *req);

void respond_register(struct kreq *req);

void respond_status(struct kreq *req);

void update_file(char *path);

void *u_thread(void *ptr);

void route();

short destroy();

int main(int argc, char **argv);

#endif
