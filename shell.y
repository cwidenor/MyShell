
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%code requires 
{

}

%union
{
  char * string;
}

%token <string> WORD PIPE
%token NOTOKEN NEWLINE STDOUT STDIN BACKGROUND REDIR_ERR REDIR_BOTH APPEND_OUT
%token APPEND_BOTH

%{

#include <stdbool.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <regex.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>

#include "command.h"
#include "single_command.h"
#include "shell.h"

void yyerror(const char * s);
int yylex();
void expandWildcard(char *, char *);
void sort();

char ** args_to_sort;
int num_args = 0;
int max_args = 5;

%}

%%

goal:
  entire_command_list
  ;

entire_command_list:
  entire_command_list entire_command
  | entire_command
  ;

entire_command:
  single_command_list io_modifier_list background NEWLINE {
    execute_command(g_current_command);
    g_current_command = malloc(sizeof(command_t));
    create_command(g_current_command);
  }
  |  NEWLINE {
    print_prompt();
  }
  ;

single_command_list:
     single_command_list PIPE single_command
  |  single_command
  ;

single_command:
    executable argument_list {
      insert_single_command(g_current_command, g_current_single_command);
      g_current_single_command = NULL;
    }
  ;

argument_list:
     argument_list argument
  |  /* can be empty */
  ;

argument:
     WORD {
       expandWildcard(NULL,$1);
       if (args_to_sort[0] == NULL) {
         insert_argument(g_current_single_command, $1);
       }
       else {
         sort();
         for (int i = 0; i < num_args; i++) {
           insert_argument(g_current_single_command, args_to_sort[i]);
         }
       }
       free(args_to_sort);
       args_to_sort = NULL;
       num_args = 0;
     }
  ;

executable:
     WORD {

       g_current_single_command = malloc(sizeof(single_command_t));
       create_single_command(g_current_single_command);
       insert_argument(g_current_single_command, $1);
     }
  ;

io_modifier_list:
     io_modifier_list io_modifier
  |  /* can be empty */   
  ;

io_modifier:
     STDOUT WORD {
       if (g_current_command->out_file) {
         creat(g_current_command->out_file, 0666);
         printf("Ambiguous output redirect.\n");
       }
       g_current_command->out_file = $2;
     }
     | STDIN WORD {
       g_current_command->in_file = $2;
     }
     | REDIR_ERR WORD {
       g_current_command->err_file = $2;
     }
     | REDIR_BOTH WORD {
       g_current_command->out_file = strdup($2);
       g_current_command->err_file = $2;
     }
     | APPEND_OUT WORD {
       g_current_command->out_file = $2;
       g_current_command->append_out = true;
     }
     | APPEND_BOTH WORD {
       g_current_command->out_file = strdup($2);
       g_current_command->err_file = $2;
       g_current_command->append_out = true;
       g_current_command->append_err = true;
     }
  ;

background:
     BACKGROUND {
       g_current_command->background=true;
     }
     |
  ;

%%

void
yyerror(const char * s)
{
  fprintf(stderr,"%s", s);
}

/*
 * This function takes two character pointers and expands any wildcards found in
 * them and then adds them to an array in order to sort them before insertion
 */

void expandWildcard(char * prefix, char * suffix) {
  /* allocate memory for the array if there isn't */
  if (args_to_sort == NULL) {
    args_to_sort = (char **)malloc(max_args * sizeof(char *));
  }

  /* add prefix to the array if there is no suffix */
  if (suffix[0] == 0) {
    /* make sure there is room for the new argument */
    if (num_args == max_args) {
      max_args = max_args * 2;
      args_to_sort = (char **)realloc(args_to_sort, max_args * sizeof(char *));
    }
    args_to_sort[num_args] = strdup(prefix);
    num_args++;
    return;
  }

  /* find the next slash if there is one */
  char * next_slash = strchr(suffix, '/');
  fprintf(stdout, "test");
  fflush(stdout);
  /* separate the suffix into parts if there is another slash */
  char next_part[1024] = "";
  if (next_slash != NULL) {
    if (next_slash - suffix != 0) {
      strncpy(next_part, suffix, next_slash-suffix);
      next_part[strlen(suffix) -strlen(next_slash)] = '\0';
    } else {
      next_part[0] = '\0';
    }
    suffix = next_slash + 1;
  }
  else {
    strcpy(next_part, suffix);
    suffix = suffix + strlen(suffix);
  } 
  fprintf(stdout, "meme");
  fflush(stdout);

  /* create a new prefix for the expanded wildcards */
  char new_prefix[1024];
  if ((strchr(next_part, '*') == NULL) && (strchr(next_part, '?') == NULL)) {
    if (prefix == NULL && prefix[0] != 0) {
      sprintf(new_prefix, "%s", next_part);
    }
    else if (next_part[0] != '\0') {
      sprintf(new_prefix, "%s/%s", prefix, next_part);
    }
    if (next_part[0] != '\0') {
      expandWildcard(new_prefix, suffix);
    } else {
      expandWildcard("", suffix);
    }
    return;
  }
  fprintf(stdout, "bruh");
  fflush(stdout);

  char * reg = (char*)malloc(2 * strlen(next_part) + 10);
  char * arg_pos = next_part;
  char * r = reg;
  *r = '^';
  r++;
  /* do the expansion */
  while (*arg_pos) {
    if (*arg_pos == '*') {
      *r = '.';
      r++;
      *r = '*';
      r++;
    }
    else if (*arg_pos == '?') {
      *r = '.';
      r++;
    }
    else if (*arg_pos == '.') {
      *r = '\\';
      r++;
      *r = '.';
      r++;
    }
    else if (*arg_pos == '/') {
    } 
    else {
      *r = *arg_pos;
      r++;
    }
    arg_pos++;
  }
  fprintf(stdout, "test");
  fflush(stdout);
  *r = '$';
  r++;
  *r = 0;
  regex_t re = {0};
  if (regcomp(&re, reg, REG_EXTENDED | REG_NOSUB) != 0) {
    return;
  }
  char * dirName = NULL;
  if (prefix == NULL) {
    dirName = strdup(".");
  }
  else if (prefix == "") {
    fprintf(stdout, "test");
    fflush(stdout);
    dirName = strdup("/");
  }
  else {
    dirName = prefix;
  }
  DIR * dir = opendir(dirName);
  if (dir == NULL) {
    return;
  }
  struct dirent * ent = {0};
  while((ent = readdir(dir)) != NULL) {
    if (regexec(&re, ent->d_name, (size_t)0, NULL, 0) == 0) {
      if (prefix == NULL || prefix[0] == 0) {
        sprintf(new_prefix, "%s", ent->d_name);
      } 
      else {
        sprintf(new_prefix, "%s/%s", prefix, ent->d_name);
      }
      if (ent->d_name[0] == '.') {
        if(next_part[0] == '.') {
          expandWildcard(new_prefix, suffix);
        }
      }
      else {
        expandWildcard(new_prefix, suffix);
      }
    }
  }
  closedir(dir);
} /* expandWildcard() */

/*
 * This function will sort the arguments to be inserted after wildcard expansion
 */

void sort() {
  for (int i = 0; i < num_args; i++) {
    for (int j = 0; j < num_args; j++) {
      if (strcmp(args_to_sort[i], args_to_sort[j]) < 0) {
        char * temp = args_to_sort[i];
        args_to_sort[i] = args_to_sort[j];
        args_to_sort[j] = temp;
      } 
    }
  }
} /* sort() */

#if 0
main()
{
  yyparse();
}
#endif
