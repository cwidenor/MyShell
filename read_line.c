#include "read_line.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "tty_raw_mode.h"

//extern void tty_raw_mode(void);

int g_line_length = 0;
char g_line_buffer[MAX_BUFFER_LINE];
int g_position = 0;

// Simple history array
// This history does not change.
// Yours have to be updated.
int g_history_index = 0;
char ** g_history = NULL;
int g_history_length = 0;

/*
 *  Prints usage for read_line
 */

void read_line_print_usage() {
  char * usage = "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character\n"
    " up arrow     See last command in the history\n";

  write(1, usage, strlen(usage));
} /* read_line_print_usage() */

/*
 * Input a line with some basic editing.
 */

char *read_line() {

  // Set terminal in raw mode
  tty_raw_mode();

  g_line_length = 0;
  g_position = 0;

  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch = '\n';
    read(0, &ch, 1);

    if ((ch >= 32) && (ch < 127)) {
      // It is a printable character.

      // Do echo
      write(1, &ch, 1);

      // If max number of character reached return.
      if (g_line_length == (MAX_BUFFER_LINE - 2)) {
        break;
      }
      for (int i = g_line_length + 1; i > g_position; i--) {
        g_line_buffer[i] = g_line_buffer[i - 1];
      }

      // add char to buffer.
      g_line_buffer[g_position] = ch;
      g_line_length++;
      g_position++;
      for (int i = g_position; i < g_line_length; i++) {
        ch = g_line_buffer[i];
        write(1, &ch, 1);
      }
      ch = 32;
      write(1, &ch, 1);
      ch = 8;
      write(1, &ch, 1);
      for (int i = g_position; i < g_line_length; i++) {
        ch = 27;
        write(1, &ch, 1);
        ch = 91;
        write(1, &ch, 1);
        ch = 68;
        write(1, &ch, 1);
      }
    }
    else if (ch == 10) {
      // <Enter> was typed. Return line
      // Print newline
      if (g_history_length == 0) {
        g_history = malloc(sizeof(char *) * 100);
      }
      g_history[g_history_length] = malloc(sizeof(char) * MAX_BUFFER_LINE);
      strncpy(g_history[g_history_length], g_line_buffer, g_line_length);
      g_history_length++;
      write(1, &ch, 1);
      break;
    }
    else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      g_line_buffer[0] = 0;
      break;
    }
    else if ((ch == 127) || (ch == 8)) {
      if (g_position <= 0) {
        continue;
      }
      for (int i = g_position; i < g_line_length; i++) {
        g_line_buffer[i - 1] = g_line_buffer[i];
      }
      g_line_length--;
      g_position--;
      ch = 32;
      write(1, &ch, 1);
      ch = 8;
      write(1, &ch, 1);
      ch = 8;
      write(1, &ch, 1);
      for (int i = g_position; i < g_line_length; i++) {
        ch = g_line_buffer[i];
        write(1, &ch, 1);
      }
      ch = 32;
      write(1, &ch, 1);
      ch = 8;
      write(1, &ch, 1);
      for (int i = g_position; i < g_line_length; i++) {
        ch = 27;
        write(1, &ch, 1);
        ch = 91;
        write(1, &ch, 1);
        ch = 68;
        write(1, &ch, 1);
      }
    }
    else if (ch == 4) {
      if (g_position == g_line_length) {
        continue;
      }
      ch = 32;
      write(1, &ch, 1);
      ch = 8;
      write(1, &ch, 1);
      for (int i = g_position; i < g_line_length - 1; i++) {
        ch = g_line_buffer[i];
        write(1, &ch, 1);
      }
      g_line_length--;
      for (int i = g_position; i < g_line_length; i++) {
        ch = g_line_buffer[i];
        write(1, &ch, 1);
      }
      ch = 32;
      write(1, &ch, 1);
      ch = 8;
      write(1, &ch, 1);
      for (int i = g_position; i < g_line_length; i++) {
        ch = 27;
        write(1, &ch, 1);
        ch = 91;
        write(1, &ch, 1);
        ch = 68;
        write(1, &ch, 1);
      }
    }
    else if (ch == 1) {
      if (g_position == 0) {
        continue;
      }
      for (int i = 0; i < g_position; i++) {
        ch = 27;
        write(1, &ch, 1);
        ch = 91;
        write(1, &ch, 1);
        ch = 68;
        write(1, &ch, 1);
      }
      g_position = 0;
    }
    else if (ch == 5) {
      if (g_line_length == g_position) {
        continue;
      }
      for (int i = g_position; i < g_line_length; i++) {
        ch = 27;
        write(1, &ch, 1);
        ch = 91;
        write(1, &ch, 1);
        ch = 67;
        write(1, &ch, 1);
      }
      g_position = g_line_length;
    }
    else if (ch == 27) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1 = '\0';
      char ch2 = '\0';
      read(0, &ch1, 1);
      read(0, &ch2, 1);
      if ((ch1 == 91) && (ch2 == 65)) {
        // Up arrow. Print next line in history.
        if (g_history_length == 0) {
          continue;
        }
        // Erase old line
        // Print backspaces
        int i = 0;
        for (i = 0; i < g_line_length; i++) {
          ch = 8;
          write(1, &ch, 1);
        }

        // Print spaces on top
        for (i = 0; i < g_line_length; i++) {
          ch = ' ';
          write(1, &ch, 1);
        }

        // Print backspaces
        for (i = 0; i < g_line_length; i++) {
          ch = 8;
          write(1, &ch, 1);
        }
        if (g_history_index < g_history_length) {
          g_history_index++;
        }
        // Copy line from history
        strcpy(g_line_buffer, g_history[g_history_length - g_history_index]);
        g_line_length = strlen(g_line_buffer);

        // echo line
        write(1, g_line_buffer, g_line_length);
        g_position = g_line_length;
      }
      else if ((ch1 == 91) && (ch2 == 66)) {
        for (int i = 0; i < g_line_length; i++) {
          ch = 8;
          write(1, &ch, 1);
        }
        for (int i = 0; i < g_line_length; i++) {
          ch = ' ';
          write(1, &ch, 1);
        }
        for ( int i = 0; i < g_line_length; i++) {
          ch = 8;
          write(1, &ch, 1);
        }
        if (g_history_index - 1 > 0) {
          g_history_index--;
          strcpy(g_line_buffer, g_history[g_history_length - g_history_index]);
        } else {
          strcpy(g_line_buffer, "");
        }
        g_line_length = strlen(g_line_buffer);
        write(1, g_line_buffer, g_line_length);
        g_position = g_line_length;
      }
      else if ((ch1 == 91) && (ch2 == 68)) {
        if (g_position == 0) {
          continue;
        }
        ch = 27;
        write(1, &ch, 1);
        ch = 91;
        write(1, &ch, 1);
        ch = 68;
        write(1, &ch, 1);
        g_position--;
      }
      else if ((ch1 == 91) && (ch2 == 67)) {
        if (g_position == g_line_length) {
          continue;
        }
        ch = 27;
        write(1, &ch, 1);
        ch = 91;
        write(1, &ch, 1);
        ch = 67;
        write(1, &ch, 1);
        g_position++;
      }
      else if ((ch1 == 91) && (ch2 == 51)) {
        char ch3 = '\n';
        read(0, &ch3, 1);
        if (ch3 == 126) {
          if (g_position == g_line_length) {
            continue;
          }
          ch = 32;
          write(1, &ch, 1);
          ch = 8;
          write(1, &ch, 1);
          for (int i = g_position; i < g_line_length - 1; i++) {
            g_line_buffer[i] = g_line_buffer[i + 1];
          }
          g_line_length--;
          for (int i = g_position; i < g_line_length; i++) {
            ch = g_line_buffer[i];
            write(1, &ch, 1);
          }
          ch = 32;
          write(1, &ch, 1);
          ch = 8;
          write(1, &ch, 1);
          for (int i = g_position; i < g_line_length; i++) {
            ch = 27;
            write(1, &ch, 1);
            ch = 91;
            write(1, &ch, 1);
            ch = 68;
            write(1, &ch, 1);
          }
        }
      }
      else if ((ch1 == 91) && (ch2 == 49)) {
        char ch3 = '\n';
        read(0, &ch3, 1);
        if (ch3 == 126) {
          if (g_position == 0) {
            continue;
          }
          for (int i = 0; i < g_position; i++) {
            ch = 27;
            write(1, &ch, 1);
            ch = 91;
            write(1, &ch, 1);
            ch = 68;
            write(1, &ch, 1);
          }
          g_position = 0;
        }
      }
      else if ((ch1 == 91) && (ch2 == 52)) {
        char ch3 = '\n';
        read(0, &ch3, 1);
        if (ch3 == 126) {
          if (g_position == g_line_length) {
            continue;
          }
          for (int i = g_position; i < g_line_length; i++) {
            ch = 27;
            write(1, &ch, 1);
            ch = 91;
            write(1, &ch, 1);
            ch = 67;
            write(1, &ch, 1);
          }
          g_position = g_line_length;
        }
      }
    }
  }

  // Add eol and null char at the end of string
  g_line_buffer[g_line_length] = 10;
  g_line_length++;
  g_line_buffer[g_line_length] = 0;

  return g_line_buffer;
} /* read_line() */

