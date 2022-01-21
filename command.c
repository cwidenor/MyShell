/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include "command.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

#include "shell.h"

extern char **environ;
int prev_background = 0;
int prev_exit = 0;
char * prev_arg;

/*
 *  Initialize a command_t
 */

void create_command(command_t *command) {
  command->single_commands = NULL;

  command->out_file = NULL;
  command->in_file = NULL;
  command->err_file = NULL;

  command->append_out = false;
  command->append_err = false;
  command->background = false;

  command->num_single_commands = 0;
} /* create_command() */

/*
 *  Insert a single command into the list of single commands in a command_t
 */

void insert_single_command(command_t *command, single_command_t *simp) {
  if (simp == NULL) {
    return;
  }

  command->num_single_commands++;
  int new_size = command->num_single_commands * sizeof(single_command_t *);
  command->single_commands = (single_command_t **)
                              realloc(command->single_commands,
                                      new_size);
  command->single_commands[command->num_single_commands - 1] = simp;
} /* insert_single_command() */

/*
 *  Free a command and its contents
 */

void free_command(command_t *command) {
  for (int i = 0; i < command->num_single_commands; i++) {
    free_single_command(command->single_commands[i]);
  }

  free(command->single_commands);

  if (command->out_file) {
    free(command->out_file);
    command->out_file = NULL;
  }

  if (command->in_file) {
    free(command->in_file);
    command->in_file = NULL;
  }

  if (command->err_file) {
    free(command->err_file);
    command->err_file = NULL;
  }

  command->append_out = false;
  command->append_err = false;
  command->background = false;

  free(command);
} /* free_command() */

/*
 *  Print the contents of the command in a pretty way
 */

void print_command(command_t *command) {
  printf("\n\n");
  printf("              COMMAND TABLE                \n");
  printf("\n");
  printf("  #   single Commands\n");
  printf("  --- ----------------------------------------------------------\n");

  // iterate over the single commands and print them nicely
  for (int i = 0; i < command->num_single_commands; i++) {
    printf("  %-3d ", i );
    print_single_command(command->single_commands[i]);
  }

  printf( "\n\n" );
  printf( "  Output       Input        Error        Background\n" );
  printf( "  ------------ ------------ ------------ ------------\n" );
  printf( "  %-12s %-12s %-12s %-12s\n",
            command->out_file?command->out_file:"default",
            command->in_file?command->in_file:"default",
            command->err_file?command->err_file:"default",
            command->background?"YES":"NO");
  printf( "\n\n" );
} /* print_command() */

/*
 *  Execute a command
 */

void execute_command(command_t *command) {
  // Don't do anything if there are no single commands
  if (command->single_commands == NULL) {
    if (isatty(0)) {
      print_prompt();
    }
    return;
  }

  if (strcmp(command->single_commands[0]->arguments[0], "exit") == 0) {
    prev_exit = 0;
    exit(0);
  }

  //save input/output/error fds for later
  int tmpin = dup(0);
  int tmpout = dup(1);
  int tmperr = dup(2);
  //set the initial input, output, and error
  int fdin;
  int fdout;
  int fderr;

  if (command->in_file) {
    fdin = open(command->in_file, O_RDONLY);
  }
  else {
    //Use default input
    fdin = dup(tmpin);
  }
  
  //setup the error file based on whether or not it should be appended
  if (command->err_file) {
    if (command->append_err) {
      fderr = open(command->err_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
    }
    else {
      fderr = open(command->err_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    }
  }
  else {
    //Use default error
    fderr = dup(tmperr);
  }

  dup2(fderr, 2);
  close(fderr);

  int ret;

  //loop through all of the commands and execute them
  for (int i = 0; i <command-> num_single_commands; i++) {
    prev_arg =
      strdup(command->single_commands[i]->arguments[command->single_commands[i]
          ->num_args - 1]);
    //redirect input
    dup2(fdin, 0);
    close(fdin);
    //setup output
    if (i == command->num_single_commands - 1) {
      //Last single command
      if (command->out_file) {
        if (command->append_out) {
          fdout = open(command->out_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
        }
        else {
          fdout = open(command->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        }
      }
      else {
        //Use default output
        fdout = dup(tmpout);
      }
    }
    else {
      //Not last single command
      //Create pipe
      int fdpipe[2];
      pipe(fdpipe);
      fdout = fdpipe[1];
      fdin = fdpipe[0];
    } //if/else
    //Redirect output
    dup2(fdout,1);
    close(fdout);

    //check for special case commands with specific executions

    //execute setenv
    if (strcmp(command->single_commands[i]->arguments[0], "setenv") == 0) {
      setenv(command->single_commands[i]->arguments[1],
          command->single_commands[i]->arguments[2], 1);
      if (isatty(0)) {
        print_prompt();
      }
      return;
    }

    //execute unsetenv
    if (strcmp(command->single_commands[i]->arguments[0], "unsetenv") == 0) {
      unsetenv(command->single_commands[i]->arguments[1]);
      if(isatty(0)) {
        print_prompt();
      }
      return;
    }

    //execute cd
    if (strcmp(command->single_commands[i]->arguments[0], "cd") == 0) {
      int ret;
      if (command->single_commands[i]->num_args > 1) {
        ret = chdir(command->single_commands[i]->arguments[1]);
      }
      else {
        ret = chdir(getenv("HOME"));
      }
      if (ret != 0) {
        fprintf(stderr, "cd: can't cd to %s", getenv("HOME"));
      }
      if (isatty(0)) {
        print_prompt();
      }
      return;
    }

    //Create child process
    command->single_commands[i]->arguments = (char
        **)realloc(command->single_commands[i]->arguments,
          (command->single_commands[i]->num_args + 1) * sizeof(char *));
    command->single_commands[i]->arguments[command->single_commands[i]->num_args] = NULL;
    ret = fork();
    if (ret == 0) {
      //printenv needs to be after the fork
      //execute printenv
      if (strcmp(command->single_commands[i]->arguments[0], "printenv") == 0) {
        char ** env = environ;
        int i = 0;
        while (env[i] != NULL) {
          fprintf(stdout, "%s\n", env[i]);
          i++;
        }
        prev_exit = 0;
        exit(0);
      }

      execvp(command->single_commands[i]->arguments[0],
          command->single_commands[i]->arguments);
      perror("execvp");
      prev_exit = 1;
      exit(1);
    }
  } //for
  //check for background process
  if (!command->background) {
    waitpid(ret, NULL, 0);
  }
  //set variable for use in special environment variable case
  if (command->background) {
    prev_background = 1;
  }
  else {
    prev_background = 0;
  }
  //restore in/out defaults
  dup2(tmpin, 0);
  dup2(tmpout, 1);
  dup2(tmperr, 2);
  close(tmpin);
  close(tmpout);
  close(tmperr);

  // Clear to prepare for next command
  free_command(command);

  // Print new prompt
  if (isatty(0)) {
    print_prompt();
  }
} /* execute_command() */
