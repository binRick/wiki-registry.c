/******************************************************************/
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/******************************************************************/
#include "../carrays/carrays.c"
#include "../cstructs-json/json/json.h"
//#include "src/wiki-commander.c"
#include "include/commander.h"
#define SKIP_LIST_C    1
#define MAX_STARS      5
/******************************************************************/
#include "wiki-registry.h"
/******************************************************************/
#define DEFAULT_URL                  "https://github.com/clibs/clib/wiki/Packages"
#define LOCAL_PACKAGES_CACHE_FILE    "./.cache-packages.txt"
#define STARS_URL                    "https://github.com/binRick?tab=stars"
/******************************************************************/
#include "src/args.c"
/******************************************************************/
list_t *pkgs;


/******************************************************************/


void stars(){
  struct parsed_stars_result *Result = parse_stars_html(get_stars_url(STARS_URL));

  fprintf(stderr, "Acquired %d List Items\n", Result->list_items->len);

  list_iterator_t *it = list_iterator_new(Result->list_items, LIST_HEAD); list_node_t *node;
  int             i = 0;

  while ((node = list_iterator_next(it))) {
    char                      *url        = get_star_url(node->val);
    struct parsed_star_result *StarResult = parse_star_html(node->val, url);
    fprintf(stderr,
            "Star #%d> %s\n"
            "- URL:           %s\n"
            "- # Pages:       %d\n"
            "- Qty:           %d\n"
            "- # Repos:       %d\n"
            "- HTML Length:   %s\n",
            ++i,
            node->val,
            StarResult->url,
            StarResult->pages,
            StarResult->qty,
            StarResult->repos->len,
            bytes_to_string(strlen(StarResult->html))
            );

    list_iterator_t *repo_it = list_iterator_new(StarResult->repos, LIST_HEAD);
    list_node_t     *repo_node; int ii = 0;
    while ((repo_node = list_iterator_next(repo_it))) {
      repo_t *Repo = (repo_t *)(repo_node->val);
      fprintf(stderr,
              "  - Repo #%d\n"
              "               Name:     %s\n"
              "               Author:   %s\n"
              "               Url:      %s\n",
              ++ii,
              Repo->name,
              Repo->author,
              Repo->url
              );
    }
    if (i >= MAX_STARS) {
      exit(0);
    }
  }
  list_iterator_destroy(it);
} /* dev */


void save_wiki(){
  printf("saving wiki....\n");
  pkgs = wiki_registry(DEFAULT_URL);
  list_node_t     *node;
  list_iterator_t *it = list_iterator_new(pkgs, LIST_HEAD);

  while ((node = list_iterator_next(it))) {
    wiki_package_t *pkg = (wiki_package_t *)node->val;
    printf("%s\n",
           pkg->repo
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
  list_node_t     *node;
  list_iterator_t *it = list_iterator_new(pkgs, LIST_HEAD);

  while ((node = list_iterator_next(it))) {
    wiki_package_t *pkg = (wiki_package_t *)node->val;
    printf("[package]\n"
           "  repo: %s\n"
           "  description: %s\n"
           "  category: %s\n"
           "  author: %s\n"
           "  project: %s\n"
           "  href: %s\n"
           "  urls: \n"
           "    - c stars: %s\n"
           "    - c repos: %s\n",
           pkg->repo,
           pkg->description,
           pkg->category,
           pkg->author,
           pkg->project,
           pkg->href,
           pkg->urls->c_stars,
           pkg->urls->c_repos
           );
    wiki_package_free(pkg);
  }
  list_iterator_destroy(it);
}


int main(const int argc, char **argv) {
  cmd = malloc(sizeof(command_t));
  command_init(cmd, argv[0], "0.0.1");
  command_option(cmd, "-v", "--verbose", "enable verbose stuff", verbose);  command_option(&cmd, "-r", "--required <arg>", "required arg", required);
  command_option(cmd, "-o", "--optional [arg]", "optional arg", optional);
  command_option(cmd, "-l", "--list [list]", "specified list", set_specified_list);
  command_parse(cmd, argc, argv);
  printf("additional args:\n");
  for (int i = 0; i < cmd->argc; ++i) {
    printf("  - '%s'\n", cmd->argv[i]);
  }

  if (argc > 1 && (strcmp(argv[1], "fetch") == 0)) {
    fprintf(stderr, "fetching....\n");
    fetch_wiki();
  }else if (argc > 1 && (strcmp(argv[1], "stars") == 0)) {
    fprintf(stderr, "stars....\n");
    stars();
  }else if (argc > 1 && (strcmp(argv[1], "b64") == 0)) {
    fprintf(stderr, "b64....\n");
    do_b64();
  }else if (argc > 1 && (strcmp(argv[1], "encode") == 0)) {
    fprintf(stderr, "encoded md5 of '%s': '%s'\n",
            "abc123",
            encoded_md5("abc123")
            );
  }else if (argc > 1 && (strcmp(argv[1], "md5") == 0)) {
    fprintf(stderr, "md5....\n");
    do_md5();
  }else if (argc > 1 && (strcmp(argv[1], "sha256") == 0)) {
    fprintf(stderr, "sha....\n");
    do_sha256();
  }else if (argc > 2 && (strcmp(argv[1], "save") == 0)) {
    save_wiki();
  }else{
    restore_wiki();
  }
}
