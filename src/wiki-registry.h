#ifndef WIKI_REGISTRY_H
#define WIKI_REGISTRY_H 1
#define SKIP_LIST_C 1
#define C_STARS_TPL "https://github.com/%s?language=c&tab=stars"
#define C_REPOS_TPL "https://github.com/%s?tab=repositories&q=&type=&language=c&sort="

#include "list/list.h"

typedef struct urls {
    char *c_stars;
    char *c_repos;
    int c_stars_qty;
    int c_repos_qty;
} urls;

typedef struct {
  char *repo;
  char *href;
  char *author;
  char *project;
  char *description;
  char *category;
  urls *urls;
} wiki_package_t;

list_t *
wiki_registry(const char *);

list_t *
wiki_registry_parse(const char *);

void
wiki_package_free(wiki_package_t *);

#endif
