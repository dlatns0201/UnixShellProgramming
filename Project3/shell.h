#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>

#define MAX_CMD_ARG 10

#define ARG 1
#define AMPERSAND 2
#define PIPELINE 3
#define LESS 4
#define GREATER 5

#define FOREGROUND 0
#define BACKGROUND 1

void handler_func(int signo);
void makelist(char *s, const char *delimiters, char **list);
int cmd_input(const char *prompt);
void readyTo_run();
