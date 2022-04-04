#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


#include "src/wiki-registry.h"
#define DEFAULT_URL "https://github.com/clibs/clib/wiki/Packages"

list_t *pkgs;

void dev(){
    printf("dev.........\n");
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
 else
    restore_wiki();

}
