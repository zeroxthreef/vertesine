#ifndef COMMENT_H__
#define COMMENT_H__

void respond_nothing(struct kreq *req);

void respond_home(struct kreq *req);

int respond_post(struct kreq *req);

void respond_get(struct kreq *req);

short init();

void route();

short destroy();

int main(int argc, char **argv);

#endif
