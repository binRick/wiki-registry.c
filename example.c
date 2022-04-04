/******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
/******************************************************************/
#include "../carrays/carrays.c"
#include "../cstructs-json/json/json.h"
#define SKIP_LIST_C 1
/******************************************************************/
#include "wiki-registry.h"
/******************************************************************/
#define DEFAULT_URL "https://github.com/clibs/clib/wiki/Packages"
#define LOCAL_PACKAGES_CACHE_FILE "./.cache-packages.txt"
#define STARS_URL "https://github.com/binRick?tab=stars"
/******************************************************************/

list_t *pkgs;

void dev(){
    struct parsed_stars_result *Result = parse_stars_html(get_stars_url(STARS_URL));
    fprintf(stderr, "Acquired %d List Items\n", Result->list_items->len);

    list_iterator_t *it = list_iterator_new(Result->list_items, LIST_HEAD); list_node_t     *node;
    int             i = 0;

    while ((node = list_iterator_next(it))) {            
        char *url = get_star_url(node->val);
        struct parsed_star_result *StarResult = parse_star_html(node->val, url);
        fprintf(stderr, "#%d -> %s|url:%s|html len:%d|qty:%d|\n", ++i, node->val,url, strlen(StarResult->html),StarResult->qty);

        exit(0);
    }

}


void save_wiki(){
  printf("saving wiki....\n");
  pkgs = wiki_registry(DEFAULT_URL);
  list_node_t *node;
  list_iterator_t *it = list_iterator_new(pkgs, LIST_HEAD);
  while ((node = list_iterator_next(it))) {
    wiki_package_t *pkg = (wiki_package_t *) node->val;
    printf("%s\n"
        , pkg->repo
         );
    wiki_package_free(pkg);
  }
  list_iterator_destroy(it);
}
void restore_wiki(){
    printf("restoring wiki....\n");
}

void fetch_wiki(){
  pkgs = wiki_registry(DEFAULT_URL);
  list_node_t *node;
  list_iterator_t *it = list_iterator_new(pkgs, LIST_HEAD);
  while ((node = list_iterator_next(it))) {
    wiki_package_t *pkg = (wiki_package_t *) node->val;
    printf("[package]\n"
           "  repo: %s\n"
           "  description: %s\n"
           "  category: %s\n"
           "  author: %s\n"
           "  project: %s\n"
           "  href: %s\n"
           "  urls: \n"
           "    - c stars: %s\n"
           "    - c repos: %s\n"
         , pkg->repo
         , pkg->description
         , pkg->category
         , pkg->author
         , pkg->project
         , pkg->href
         , pkg->urls->c_stars
         , pkg->urls->c_repos
         );
    wiki_package_free(pkg);
  }
  list_iterator_destroy(it);
}

int main(const int argc, char **argv) {
 if(argc == 2 && (strcmp(argv[1],"fetch") == 0))
    fetch_wiki();
 else if(argc == 2 && (strcmp(argv[1],"dev") == 0))
     dev();
 else if(argc == 2 && (strcmp(argv[1],"save") == 0)){
    save_wiki();
 }else
    restore_wiki();

}
