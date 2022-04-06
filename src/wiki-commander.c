#ifndef WIKI_CC1
#define WIKI_CC1
#include "../include/commander.h"


static void verbose() {
  printf("verbose: enabled\n");
}


static void required(command_t *self) {
  printf("required: %s\n", self->arg);
}


static void set_specified_list(command_t *self) {
  printf("specified_list: %s\n", self->arg);
  // eargs->list = strdup(self->arg);
}


static void optional(command_t *self) {
  printf("optional: %s\n", self->arg);
}


static int commander_main(int argc, char **argv) {
  //eargs = malloc(sizeof(args_t));
  command_t cmd;

  command_init(&cmd, argv[0], "0.0.1");
  command_option(&cmd, "-v", "--verbose", "enable verbose stuff", verbose);
  command_option(&cmd, "-r", "--required <arg>", "required arg", required);
  command_option(&cmd, "-o", "--optional [arg]", "optional arg", optional);
  command_option(&cmd, "-l", "--list [list]", "specified list", set_specified_list);
  command_parse(&cmd, argc, argv);
  printf("additional args:\n");
  for (int i = 0; i < cmd.argc; ++i) {
    printf("  - '%s'\n", cmd.argv[i]);
  }
  command_free(&cmd);
  return(0);
}
#endif
