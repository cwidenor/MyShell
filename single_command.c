#include "single_command.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>

#include "shell.h"

/*
 *  Initialize a single command
 */

void create_single_command(single_command_t *simp) {
  simp->arguments = NULL;
  simp->num_args = 0;
} /* create_single_command() */

/*
 *  Free everything in a single command
 */

void free_single_command(single_command_t *simp) {
  for (int i = 0; i < simp->num_args; i++) {
    free(simp->arguments[i]);
    simp->arguments[i] = NULL;
  }

  free(simp->arguments);

  free(simp);
} /* free_single_command() */

/*
 *  Insert an argument into the list of arguments in a single command
 */

void insert_argument(single_command_t *simp, char *argument) {
  if (argument == NULL) {
    return;
  }
  char * pat = "\\${.*}";
  regex_t p = {0};
  regmatch_t match = {0};
  if (regcomp(&p, pat, 0)) {
    exit(1);
  }
  if (!regexec(&p, argument, 1, &match, 0)) {
    char * expanded = (char *)calloc(1, 1024 * sizeof(char));
    int i = 0;
    int j = 0;
    while (argument[i] != '\0') {
      if (argument[i] != '$') {
        expanded[j] = argument[i];
        expanded[j + 1] = '\0';
        i++;
        j++;
      }
      else {
        char * left_curly = strchr((char *)(argument + i), '{');
        char * right_curly = strchr((char *)(argument + i), '}');
        char * en_var = (char *)calloc(1, strlen(argument) * sizeof(char));
        strncat(en_var, left_curly + 1, right_curly - left_curly - 1);
        char * actual = NULL;
        if (strcmp(en_var, "!") == 0) {
          fprintf(stdout, "%d", prev_background);
          fflush(stdout);
          actual = (char *)malloc(sizeof(char) * 2);
          sprintf(actual, "%d", prev_background);
        } else if (strcmp(en_var, "$") == 0) {
          fprintf(stdout, "%d", getpid());
          fflush(stdout);
          actual = (char *)malloc(sizeof(char) * 6);
          sprintf(en_var, "%d", getpid());
        } else if (strcmp(en_var, "?") == 0) {
          fprintf(stdout, "%d", prev_exit);
          fflush(stdout);
          actual = (char *)malloc(sizeof(char));
          sprintf(en_var, "%d", prev_exit);
        } else if (strcmp(en_var, "_") == 0) {
          fprintf(stdout, "%s", prev_arg);
          fflush(stdout);
        } else {
          actual = getenv(en_var);
          if (actual == NULL) {
            strcat(expanded, "");
          }
          else {
            strcat(expanded, actual);
          }
        }
        i += strlen(en_var) + 3;
        j += strlen(actual);
        free(en_var);
      }
    }
    argument = strdup(expanded);
  }
  if (argument[0] == '~') {
    if (strlen(argument) == 1) {
      struct passwd * pw = getpwuid(getuid());
      char * home = pw->pw_dir;
      char * expanded = (char *)malloc(strlen(home) + strlen(argument) *
            sizeof(char) + 1);
      expanded[0] = '\0';
      strcat(expanded, home);
      strcat(expanded, "/");
      strcat(expanded, (char *)(argument + 1));
      expanded[strlen(home) + strlen(argument) + 1] = '\0';
      argument = expanded;
    }
    else
    {
      struct passwd * pw = getpwnam(argument + 1);
      if (pw == NULL) {
        char * dir = argument + 1;
        char * expanded = (char *)malloc(strlen(dir) + strlen("/homes/" + 1) *
              sizeof(char));
        strcat(expanded, "/homes/");
        strcat(expanded, dir);
        expanded[strlen(dir) + strlen("/homes/")] = '\0';
        argument = expanded;
      }
      else {
        argument = strdup(pw->pw_dir);
      }
    }
  }
  regfree(&p);
  simp->num_args++;
  simp->arguments = (char **)realloc(simp->arguments,
                                    simp->num_args * sizeof(char *));
  simp->arguments[simp->num_args - 1] = argument;
} /* insert_argument() */

/*
 *  Print a single command in a pretty format
 */

void print_single_command(single_command_t *simp) {
  for (int i = 0; i < simp->num_args; i++) {
    printf("\"%s\" \t", simp->arguments[i]);
  }

  printf("\n\n");
} /* print_single_command() */
