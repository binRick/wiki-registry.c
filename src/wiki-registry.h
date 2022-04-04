#ifndef WIKI_REGISTRY_H
#define WIKI_REGISTRY_H    1
#define SKIP_LIST_C        1
#define C_STARS_TPL        "https://github.com/%s?language=c&tab=stars"
#define C_REPOS_TPL        "https://github.com/%s?tab=repositories&q=&type=&language=c&sort="
#define C_REPO_TPL         "https://github.com/%s/%s"

#include "list/list.h"
#include <stdio.h>
///#include "commander.h"

typedef struct repo_t               repo_t;
typedef struct                      repo_t {
  char *name;
  char *url;
  char *author;
};
typedef struct parsed_star_result   parsed_star_result;
typedef struct                      parsed_star_result {
  char   *name;
  char   *url;
  char   *html;
  list_t *html_lines;
  list_t *repos;
  int    qty;
  char   *repo;
  char   *author;
  int    pages;
};


typedef struct parsed_stars_result {
  list_t *list_items;
};

typedef struct urls {
  char *c_stars;
  char *c_repos;
  int  c_stars_qty;
  int  c_repos_qty;
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

char *
get_star_html(char *name, char *list_name);

char *
get_star_url(char *name);

list_t *
wiki_registry_parse(const char *);

struct parsed_star_result *
parse_star_html(char *, char *);

struct parsed_stars_result *
parse_stars_html(const char *);

int
get_star_repos_qty(const char *star_name);

list_t *
get_list_items();

char *
get_star_url(char *);

void
print_parsed_star_result(struct parsed_star_result *);

char *
get_stars_url(const char *);

void
wiki_package_free(wiki_package_t *);

#endif
