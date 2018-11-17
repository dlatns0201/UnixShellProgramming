#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>

#define MAX_CMD_ARG 10

const char *prompt = "myshell> ";
char* cmdvector[MAX_CMD_ARG];
char  cmdline[BUFSIZ];

void fatal(char *str) {
	perror(str);
	exit(1);
}

int makelist(char *s, const char *delimiters, char** list, int MAX_LIST) {
	int i = 0;
	int numtokens = 0;
	char *snew = NULL;

	if ((s == NULL) || (delimiters == NULL)) return -1;

	snew = s + strspn(s, delimiters);       /* delimiters를 skip */
	if ((list[numtokens] = strtok(snew, delimiters)) == NULL)
		return numtokens;

	numtokens = 1;

	while (1) {
		if ((list[numtokens] = strtok(NULL, delimiters)) == NULL)
			break;
		if (numtokens == (MAX_LIST - 1)) return -1;
		numtokens++;
	}
	return numtokens;
}

int main(int argc, char**argv) {
	int i = 0;
	pid_t pid;
	int status;
	int cmdsize;
	while (1) {
		fputs(prompt, stdout);
		fgets(cmdline, BUFSIZ, stdin);
		cmdline[strlen(cmdline) - 1] = '\0';
		if (strcmp(cmdline, "exit") == 0) {
			exit(0);
		}
		cmdsize=makelist(cmdline, " \t", cmdvector, MAX_CMD_ARG);
		switch (pid = fork()) {
			case 0:

				if (strcmp(cmdvector[0], "cd") == 0) {
					exit(5);
				}
				if(strcmp(cmdvector[cmdsize-1],"&")==0){
					cmdvector[cmdsize-1]=NULL;
				}
				execvp(cmdvector[0], cmdvector);
				fatal("main()");
			case -1:
				fatal("main()");
			default:
				if(strcmp(cmdvector[cmdsize-1],"&")){
					waitpid(pid,&status,0);
}
				if (WIFEXITED(status)) {
					if (WEXITSTATUS(status) == 5) {
						chdir(cmdvector[1]);
					}
				}
		}
	}
	return 0;
}
