#define SKIP_LIST_C          1
#define STAR_URL_TEMPLATE    "https://github.com/stars/binRick/lists/%s"
#define C_REPO_STAR_TPL      "https://github.com/stars/%s/lists/%s"
/*************************************************/
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
//#include "commander.c"
//#include "wiki-commander.c"
/*************************************************/
#include "gumbo-get-element-by-id/get-element-by-id.h"
#include "gumbo-get-elements-by-tag-name/get-elements-by-tag-name.h"
#include "gumbo-parser/gumbo.h"
#include "gumbo-text-content/gumbo-text-content.h"
/*************************************************/
#include "http-get/http-get.h"
/*************************************************/
#include "../deps/tiny-regex-c/re.h"
/*************************************************/
#include "list/list.h"
#include "substr/substr.h"
/*************************************************/
#include "case/case.h"
#include "strdup/strdup.h"
#include "trim/trim.h"
#include "wiki-registry.h"
/*************************************************/
#include "../../string/splitqty.c"
#include "../../string/stringfn.c"
#include "../../string/strsplit.c"
/*************************************************/
#include "../../log/log.c"
#include "../../string/strconv.h"
/*************************************************/
#include "../../parson/parson.c"
#include "../../parson/parson.h"
#include "../../string/str-copy.c"
#include "../../string/str-replace.c"
#include "./structs.c"
/*************************************************/
#include "../../fs/fs.c"
#include "../../fs/fsio.c"
#include "../../fs/time.c"
#include "../../string/stringbuffer.c"
/*************************************************/
#define CACHE_ENABLED        false
#define DEFAULT_LOG_LEVEL    LOG_DEBUG
/*************************************************/
int repos_per_page = 30;
/*************************************************/
/*************************************************/
struct parsed_star_result *parse_star_html(char *name, char *url) {
  struct parsed_star_result *Result = malloc(sizeof(struct parsed_star_result));

  Result->name = name;
  Result->url  = url;
  Result->html = get_star_html(Result->name, url);

  Result->repos = list_new();
  assert(ParseStarResult(Result) == 0);
  assert(Result->qty > 0);

  return(Result);
}
/*************************************************/


static char *wiki_package_to_str(wiki_package_t *self){
  JSON_Value  *root_value  = json_value_init_object();
  JSON_Object *root_object = json_value_get_object(root_value);

  json_object_set_string(root_object, "repo", self->repo);
  char *ret = strdup(json_serialize_to_string_pretty(root_value));

  json_value_free(root_value);
  return(ret);
}


static wiki_package_t *wiki_package_new() {
  wiki_package_t *pkg = malloc(sizeof(wiki_package_t));

  if (pkg) {
    pkg->repo        = NULL;
    pkg->href        = NULL;
    pkg->description = NULL;
    pkg->author      = NULL;
    pkg->project     = NULL;
    pkg->category    = NULL;
    pkg->urls        = malloc(sizeof(urls));
  }
  return(pkg);
}


/**
 * Add `href` to the given `package`.
 */


static void add_package_href(wiki_package_t *self) {
  size_t len = strlen(self->repo) + 20; // https://github.com/ \0

  self->href = malloc(len);
  if (self->href) {
    sprintf(self->href, "https://github.com/%s", self->repo);
  }
}


char *get_repo_star_url(char *author, char *star_name) {
  char msg[1024];

  sprintf(&msg, C_REPO_STAR_TPL, author, star_name);
  return(strdup(msg));
}


char *get_repo_url(char *author, char *name) {
  char msg[1024];

  sprintf(&msg, C_REPO_TPL, author, name);
  return(strdup(msg));
}


static wiki_package_t * parse_li(GumboNode *li) {
  wiki_package_t *self = wiki_package_new();
  char           *text = NULL;

  if (!self) {
    goto cleanup;
  }

  text = gumbo_text_content(li);
  if (!text) {
    goto cleanup;
  }

  // TODO support unicode dashes
  char *tok = strstr(text, " - ");

  if (!tok) {
    goto cleanup;
  }

  int pos = tok - text;

  self->repo        = substr(text, 0, pos);
  self->description = substr(text, pos + 3, -1);
  if (!self->repo || !self->description) {
    goto cleanup;
  }
  trim(self->description);
  trim(self->repo);

  self->urls->c_stars = strdup("UNKNOWN");
  self->urls->c_repos = strdup("UNKNOWN");
  self->author        = strdup("UNKNOWN");
  self->project       = strdup("UNKNOWN");

  if (splitqty(self->repo, "/") == 2) {
    char buf[1024];
    char **split = strsplit(self->repo, "/");
    self->author  = strdup(split[0]);
    self->project = strdup(split[1]);
    sprintf(&buf, C_STARS_TPL, self->author);
    self->urls->c_stars = strdup(trim(buf));
    sprintf(&buf, C_REPOS_TPL, self->author);
    self->urls->c_repos = strdup(trim(buf));
  }

  trim(self->project);
  trim(self->author);

  log_trace("author:%s", self->author);
  log_trace("repo:%s", self->repo);
  log_trace("c stars:%s", self->urls->c_stars);

  add_package_href(self);

cleanup:
  free(text);
  return(self);
} /* parse_li */


list_t *wiki_registry_parse(const char *html) {
  log_set_level(DEFAULT_LOG_LEVEL);
  GumboOutput *output = gumbo_parse(html);
  list_t      *pkgs   = list_new();

  GumboNode   *body = gumbo_get_element_by_id("body", output->root);

  if (body) {
    // grab all category `<h2 />`s
    list_t          *h2s = gumbo_get_elements_by_tag_name("h2", body);
    list_node_t     *heading_node;
    list_iterator_t *heading_iterator = list_iterator_new(h2s, LIST_HEAD);
    while ((heading_node = list_iterator_next(heading_iterator))) {
      GumboNode *heading  = (GumboNode *)heading_node->val;
      char      *category = gumbo_text_content(heading);
      // die if we failed to parse a category, as it's
      // almost certinaly a malloc error
      if (!category) {
        break;
      }
      trim(case_lower(category));
      GumboVector *siblings = &heading->parent->v.element.children;
      size_t      pos       = heading->index_within_parent;

      // skip elements until the UL
      // TODO: don't hardcode position here
      // 2:
      //   1 - whitespace
      //   2 - actual node
      GumboNode *ul = siblings->data[pos + 2];
      if (GUMBO_TAG_UL != ul->v.element.tag) {
        free(category);
        continue;
      }

      list_t          *lis         = gumbo_get_elements_by_tag_name("li", ul);
      list_iterator_t *li_iterator = list_iterator_new(lis, LIST_HEAD);
      list_node_t     *li_node;
      while ((li_node = list_iterator_next(li_iterator))) {
        wiki_package_t *package = parse_li(li_node->val);
        if (package && package->description) {
          package->category = strdup(category);
          list_rpush(pkgs, list_node_new(package));
          if (package) {
            wiki_package_free(package);
          }
        }
        list_iterator_destroy(li_iterator);
        list_destroy(lis);
        free(category);
      }
      list_iterator_destroy(heading_iterator);
      list_destroy(h2s);
    }
  }

  gumbo_destroy_output(&kGumboDefaultOptions, output);
  return(pkgs);
} /* wiki_registry_parse */

bool       CACHE_RESPONSE  = true;
bool       CACHED_RESPONSE = true;

char       *RESPONSE_CACHE_FILE_PATH = "./.response-cache.txt";
const char *STARS_RESPONSE_FILE      = "./.stars-response.html";
const char *STAR_RESPONSE_TEMPLATE   = "./.star-%s-response.html";


char * get_star_html(char *name, char *url) {
  log_debug("Fetching star url "AC_RESETALL AC_YELLOW AC_REVERSED "%s"AC_RESETALL, url);
  char star_html_file[1024];

  sprintf(star_html_file, STAR_RESPONSE_TEMPLATE, name);
  if (fsio_file_exists(star_html_file) && (fsio_file_size(star_html_file) > 0)) {
    log_debug("cached response!");
    return(fsio_read_text_file(star_html_file));
  }else{
    log_debug("uncached response!");
  }
  http_get_response_t *res;

  res = http_get(url);
  assert(res->ok);
  log_debug("file:%s", star_html_file);
  assert(fsio_write_text_file(star_html_file, res->data));
  http_get_free(res);
  return(fsio_read_text_file(star_html_file));
}


char * get_stars_url(const char *url) {
  log_debug("Fetching url "AC_RESETALL AC_YELLOW AC_REVERSED "%s"AC_RESETALL, url);
  tq_start("");
  if (fsio_file_exists(STARS_RESPONSE_FILE) && (fsio_file_size(STARS_RESPONSE_FILE) > 0)) {
    log_debug("cached response!");
    return(fsio_read_text_file(STARS_RESPONSE_FILE));
  }else{
    log_debug("uncached response!");
  }
  http_get_response_t *res;
  bool                dofree = false;

  res = http_get(url);
  if (!res->ok) {
    return(NULL);
  }
  dofree = true;
  char *dur = tq_stop("");

  log_debug("got %s of data from url %s in %s", bytes_to_string(strlen(res->data)), url, dur);
  const char *ret = strdup(res->data);

  save_stars_html(res->data, STARS_RESPONSE_FILE);

  if (dofree) {
    http_get_free(res);
  }
  return(fsio_read_text_file(STARS_RESPONSE_FILE));
}


void save_stars_html(const char *html, const char *path){
  return(fsio_write_text_file(path, html));
}


list_t *get_list_items() {
  list_t      *list_items = list_new();
  char        *LIST_NAME  = "x";
  list_node_t *list_item  = strdup(LIST_NAME);

  list_rpush(list_items, list_node_new(list_item));
  return(list_items);
}


char *get_star_url(char *list_name) {
  char msg[1024];
  int  q = splitqty(list_name, " ");

  if (q > 0) {
    char *_list_name = strdup(list_name);
    char **s         = strsplit(list_name, " ");
    list_name = strdup(s[0]);
  }
  sprintf(&msg, STAR_URL_TEMPLATE, list_name);
  return(strdup(&msg));
}


bool AcquireStarredReposResponses(parsed_star_result *StarResult) {
  return(true);
}


bool ParseStarredReposQty(parsed_star_result *StarResult) {
  re_t repos_qty_regex = re_compile(" repositories</div>\\s*");
  char *html = strdup(StarResult->html);
  int  lines_qty = splitqty(html, "\n"), repos_qty_match_qty;
  char **lines = strsplit(html, "\n");

  assert(lines_qty > 0);
  for (int i = 0; i < lines_qty; i++) {
    char *line = lines[i];
    if (strlen(line) < 1) {
      continue;
    }
    int match_repos_qty = re_matchp(repos_qty_regex, line, &repos_qty_match_qty);
    if (match_repos_qty != -1) {
      int  sq1 = splitqty(line, ">"), sq2 = splitqty(line, "<"); assert(sq1 == 3 && sq2 == 3);
      char **ss = strsplit(line, ">");
      char *s   = ss[1];
      ss                = strsplit(s, "<"); s = ss[0];
      ss                = strsplit(s, " "); s = ss[1];
      StarResult->qty   = str_to_int32(s);
      StarResult->pages = (StarResult->qty / 30) + 1;
      return(true);
    }
  }
  return(false);
}


char **get_url_html_lines(const char *url) {
}


int ParseStarResult(parsed_star_result *StarResult) {
  assert(ParseStarredReposQty(StarResult));
  assert(AcquireStarredReposResponses(StarResult));
  char *html     = strdup(StarResult->html);
  int  lines_qty = splitqty(html, "\n");

  assert(lines_qty > 0);
  char **lines     = strsplit(html, "\n");
  re_t repos_regex = re_compile(" / </span>\\s*");

  for (int i = 0; i < lines_qty; i++) {
    char *line = lines[i];
    if (strlen(line) < 1) {
      continue;
    }
    int match_length, repos_match_qty;
    int match_repos = re_matchp(repos_regex, line, &repos_match_qty);
    if (match_repos == -1) {
      continue;
    }
    log_trace("match- '%s' > %d|%i", line, match_repos, repos_match_qty);
    char *es = str_replace(line, " ", "");
    int  sq1 = splitqty(es, ">"), sq2 = splitqty(es, "<");
    assert(sq1 > 1 && sq2 > 1);
    log_trace("repos match!|len:%i|qty:%i|>     " AC_RESETALL AC_YELLOW AC_REVERSED "%s" AC_RESETALL,
              match_repos,
              repos_match_qty,
              es
              );
    int  es_split_qty = splitqty(es, ">");
    assert(es_split_qty > 1);
    char **es_split       = strsplit(es, ">");
    char *author          = es_split[1];
    int  author_split_qty = splitqty(author, "/");
    assert(author_split_qty > 1);
    char **author_split = strsplit(author, "/");
    author = author_split[0];
    char *repo = es_split[2];

    log_trace(AC_RESETALL AC_REVERSED AC_BLUE "author:%s|repo:%s" AC_RESETALL, author, repo);

    ////////////////////////////////////////////////////
    //////    Allocate new Repo (repo_t)            ////
    ////////////////////////////////////////////////////
    repo_t *Repo = malloc(sizeof(repo_t));
    Repo->author = strdup(author);
    Repo->name   = strdup(repo);
    Repo->url    = get_repo_url(author, repo);
    list_node_t *repo_item = list_node_new(Repo);
    list_rpush(StarResult->repos, repo_item);
    ////////////////////////////////////////////////////
  }

  return(0);
} /* ParseStarResult */


void print_parsed_star_result(struct parsed_star_result *StarResult) {
  log_debug("printing............");
}

struct parsed_stars_result *parse_stars_html(const char *html) {
  log_set_level(DEFAULT_LOG_LEVEL);
  struct parsed_stars_result *Result = malloc(sizeof(struct parsed_stars_result));

  Result->list_items = list_new();
  GumboOutput     *output = gumbo_parse(html);
  list_t          *pkgs   = list_new();
  GumboNode       *body   = gumbo_get_element_by_id("profile-lists-container", output->root);
  list_t          *h3s    = gumbo_get_elements_by_tag_name("h3", body);
  list_node_t     *heading_node;
  list_iterator_t *heading_iterator = list_iterator_new(h3s, LIST_HEAD);

  while ((heading_node = list_iterator_next(heading_iterator))) {
    GumboNode *heading = (GumboNode *)heading_node->val;
    char      *item    = gumbo_text_content(heading);
    if (!item) {
      break;
    }
    list_node_t *list_item = strdup(trim(case_lower(item)));
    list_rpush(Result->list_items, list_node_new(list_item));
    free(item);
  }
  list_iterator_destroy(heading_iterator);
  list_destroy(h3s);
  gumbo_destroy_output(&kGumboDefaultOptions, output);
  return(Result);
}


list_t * wiki_registry(const char *url) {
  log_debug("Fetching url "AC_RESETALL AC_YELLOW AC_REVERSED "%s"AC_RESETALL, url);
  tq_start("");
  http_get_response_t *res;
  bool                dofree = false;

  fs_creation_time(RESPONSE_CACHE_FILE_PATH);
  if (CACHE_ENABLED && fsio_file_exists(RESPONSE_CACHE_FILE_PATH) && (fsio_file_size(RESPONSE_CACHE_FILE_PATH) > 0)) {
    res->data = fsio_read_text_file(RESPONSE_CACHE_FILE_PATH);
    log_info("Read %db from cache file %s", strlen(res->data));
  }else{
    res = http_get(url);
    if (!res->ok) {
      return(NULL);
    }
    dofree = true;
    if (CACHE_RESPONSE) {
      if (!fsio_file_exists(RESPONSE_CACHE_FILE_PATH)) {
        if (!fsio_create_empty_file(RESPONSE_CACHE_FILE_PATH)) {
          log_fatal("Failed to create cache file!");
        }
      }
      if (!fsio_write_text_file(RESPONSE_CACHE_FILE_PATH, res->data)) {
        log_fatal("Failed to write cache file!");
      }
    }
  }
  char *dur = tq_stop("");

  log_trace("got %s of data from url %s in %s", bytes_to_string(strlen(res->data)), url, dur);

  tq_start("");
  list_t *list = wiki_registry_parse(res->data);

  dur = tq_stop("");
  log_info("Parsed %s in %s", bytes_to_string(strlen(res->data)), dur);

  if (dofree) {
    http_get_free(res);
  }

  return(list);
} /* wiki_registry */


/**
 * Free a wiki_package_t.
 */


void wiki_package_free(wiki_package_t *pkg) {
  free(pkg->repo);
  free(pkg->href);
  free(pkg->description);
  free(pkg->author);
  free(pkg->project);
  free(pkg->category);
  free(pkg);
}
