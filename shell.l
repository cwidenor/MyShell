
/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "y.tab.h"

extern  char * read_line();

int mygetc(FILE * f) {
  static char *p;
  char ch;

  if (!isatty(0)) {
    // stdin is not a tty. Call real getc
    return getc(f);
  }

  // stdin is a tty. Call our read_line.
  if (p == NULL || *p == 0) {
    char * s = read_line();
    p = s;
  }

  ch = *p;
  p++;
  
  return ch;
}

#undef getc
#define getc(f) mygetc(f)

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

%}

%option noyywrap

%%

\n {
  return NEWLINE;
}

[ \t] {
}

">" {
  return STDOUT;
}

"|" {
  return PIPE;
}

"<" {
  return STDIN;
}

"2>" {
  return REDIR_ERR;
}

">&" {
  return REDIR_BOTH;
}

">>" {
  return APPEND_OUT;
}

">>&" {
  return APPEND_BOTH;
}

"&" {
  return BACKGROUND;
}

["][^"]*["] {
  yylval.string = strdup(yytext+1);
  if (yylval.string[yyleng-2] == '"') 
  {
    yylval.string[yyleng-2] = 0;
  }
  return WORD;
} 

[$][\(][^\)]*[\)] {
  char * sub = strdup(yytext + 2);
  sub[strlen(sub) - 1] = '\0';

  int tmpin = dup(0);
  int tmpout = dup(1);
  int fdin[2];
  int fdout[2];

  pipe(fdin);
  pipe(fdout);

  write(fdin[1], sub, strlen(sub));
  write(fdin[1], "\nexit\n", 6);
  close(fdin[1]);

  dup2(fdin[0], 0);
  close(fdin[0]);
  dup2(fdout[1], 1);
  close(fdout[1]);

  int ret = fork();
  char ** args = (char **)malloc(2048);
  if (ret == 0)
  {
    args[0] = sub;
    args[1] = NULL;
    execvp("/proc/self/exe", args);
  }
  else if (ret < 0)
    exit(1);

  dup2(tmpin, 0);
  dup2(tmpout, 1);
  close(tmpin);
  close(tmpout);

  char c;
  char * buff = (char *)malloc(2048);
  int i = 0;
  while (read(fdout[0], &c, 1)) {
    if (c == '\n')
      buff[i] = ' ';
    else
      buff[i] = c;
    i++;
  }

  close(fdin[0]);
  close(fdin[1]);
  close(fdout[0]);
  close(fdout[1]);
  buff[i] = '\0';

  for (i = strlen(buff) - 1; i >= 0; i--)
    unput(buff[i]);
}


[^ \t\n<>&\|]*(\\.[^ \t\n<>&\|]*)* {
  yylval.string = strdup(yytext);
  char curCommand [strlen(yylval.string) + 1];
  int i = 0;
  int j = 0;
  for(i = i; i < strlen(yylval.string); i++)
  {
    if (yylval.string[i] != '\\') {
      curCommand[j] = yylval.string[i];
      j++;
    }
    else {
      curCommand[j] = yylval.string[i + 1];
      i++;
      j++;
    }
  }
  curCommand[j] = '\0';
  strcpy(yylval.string, curCommand);
  return WORD;
}
