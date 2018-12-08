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
#define EOL 6

#define FOREGROUND 0
#define BACKGROUND 1

void handler_func(int signo);
int cmd_input(const char *prompt);
void readyTo_run();
