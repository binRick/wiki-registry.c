#include "../include/commander.h"
#include <stdio.h>


static command_t *cmd;


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
