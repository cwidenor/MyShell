#include "shell.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "command.h"
#include "single_command.h"

command_t *g_current_command = NULL;
single_command_t *g_current_single_command = NULL;

int yyparse(void);

/*
 *  Prints shell prompt
 */

void print_prompt() {
  printf("myshell>");
  fflush(stdout);
} /* print_prompt() */

/*
 * This function will exit the shell when ctrl-C is pressed
 */

extern void sig_handler_ctrlC(int sig) {
  if (sig == SIGINT) {
    exit(0);
  }
} /* sig_handler_ctrlC() */

/*
 * This function will exit all of the zombie processes
 */

extern void sig_handler_zombie() {
  pid_t pid = wait3(0, 0, NULL);
  while (waitpid(-1, NULL, WNOHANG) > 0) {
    printf("\n[%d] exited. ", pid);
  }
} /* sig_handler_zombie() */

/*
 *  This main is simply an entry point for the program which sets up
 *  memory for the rest of the program and the turns control over to
 *  yyparse and never returns
 */

int main() {
  struct sigaction sa_c = {0};
  sa_c.sa_handler = sig_handler_ctrlC;
  sigemptyset(&sa_c.sa_mask);
  sa_c.sa_flags = SA_RESTART;
  if (sigaction(SIGINT, &sa_c, NULL)) {
    exit(0);
  }

  struct sigaction sa_zombie = {0};
  sa_zombie.sa_handler = sig_handler_zombie;
  sigemptyset(&sa_zombie.sa_mask);
  sa_zombie.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa_zombie, NULL)) {
    exit(1);
  }

  g_current_command = (command_t *)malloc(sizeof(command_t));
  g_current_single_command =
        (single_command_t *)malloc(sizeof(single_command_t));

  create_command(g_current_command);
  create_single_command(g_current_single_command);
  if (isatty(0)) {
    print_prompt();
  }
  yyparse();
} /* main() */
