#define SKIP_LIST_C          1
#define STAR_URL_TEMPLATE    "https://github.com/stars/binRick/lists/%s"
/*************************************************/
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
/*************************************************/
#include "gumbo-parser/gumbo.h"
#include "gumbo-text-content/gumbo-text-content.h"
#include "gumbo-get-element-by-id/get-element-by-id.h"
#include "gumbo-get-elements-by-tag-name/get-elements-by-tag-name.h"
/*************************************************/
#include "http-get/http-get.h"
/*************************************************/
#include "../deps/tiny-regex-c/re.h"
/*************************************************/
#include "list/list.h"
#include "substr/substr.h"
/*************************************************/
#include "strdup/strdup.h"
#include "case/case.h"
#include "trim/trim.h"
#include "wiki-registry.h"
/*************************************************/
#include "../../string/strsplit.c"
#include "../../string/splitqty.c"
/*************************************************/
#include "../../log/log.c"
/*************************************************/
#include "../../string/str-copy.c"
#include "../../string/str-replace.c"
#include "../../parson/parson.h"
#include "../../parson/parson.c"
#include "./structs.c"
/*************************************************/
#include "../../string/stringbuffer.c"
#include "../../fs/fs.c"
#include "../../fs/fsio.c"
#include "../../fs/time.c"
/*************************************************/
#define CACHE_ENABLED        false
#define DEFAULT_LOG_LEVEL    LOG_DEBUG
/**
 * Create a new wiki package.
 */


static char *wiki_package_to_str(wiki_package_t *self){
  JSON_Value  *root_value  = json_value_init_object();
  JSON_Object *root_object = json_value_get_object(root_value);

  json_object_set_string(root_object, "repo", self->repo);
  char *ret = strdup(json_serialize_to_string_pretty(root_value));

  json_value_free(root_value);
  return(ret);
}


static wiki_package_t *wiki_package_new()                        {
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


/**
 * Parse the given wiki `li` into a package.
 */


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


list_t *wiki_registry_parse(const char *html)         {
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
        } else {
          // failed to parse package
          if (package) {
            wiki_package_free(package);
          }
        }
      }
      list_iterator_destroy(li_iterator);
      list_destroy(lis);
      free(category);
    }
    list_iterator_destroy(heading_iterator);
    list_destroy(h2s);
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


list_t *get_list_items()         {
  list_t      *list_items = list_new();
  char        *LIST_NAME  = "x";
  list_node_t *list_item  = strdup(LIST_NAME);

  list_rpush(list_items, list_node_new(list_item));
  return(list_items);
}


char *get_star_url(char *list_name)       {
  char msg[1024];

  sprintf(&msg, STAR_URL_TEMPLATE, list_name);
  return(strdup(&msg));
}


void dev_regex(){
  /* Standard int to hold length of match */
  int match_length;

/* Standard null-terminated C-string to search: */
  const char *string_to_search = "ahem.. 'hello world !' ..";

/* Compile a simple regular expression using character classes, meta-char and greedy + non-greedy quantifiers: */
  re_t pattern = re_compile("[Hh]ello [Ww]orld\\s*[!]?");

/* Check if the regex matches the text: */
  int match_idx = re_matchp(pattern, string_to_search, &match_length);

  if (match_idx != -1) {
    printf("match at idx %i, %i chars long.\n", match_idx, match_length);
  }
}


struct parsed_star_result *parse_star_html(char *name, char *url)                            {
  struct parsed_star_result *Result = malloc(sizeof(struct parsed_star_result));

  Result->name = name;
  Result->url  = url;
  Result->html = get_star_html(Result->name, url);
  Result->qty  = 666;

  return(Result);
}


void print_parsed_star_result(struct parsed_star_result *StarResult)      {
  log_debug("printing............");
}

struct parsed_stars_result *parse_stars_html(const char *html)                             {
  struct parsed_stars_result *Result = malloc(sizeof(struct parsed_stars_result));

  Result->list_items = list_new();
  log_set_level(DEFAULT_LOG_LEVEL);
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


void wiki_package_free(wiki_package_t *pkg)      {
  free(pkg->repo);
  free(pkg->href);
  free(pkg->description);
  free(pkg->author);
  free(pkg->project);
  free(pkg->category);
  free(pkg);
}
