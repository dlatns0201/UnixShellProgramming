#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>

#define MAX_CMD_ARG 10

const char *prompt = "myshell> ";
char* cmdvector[MAX_CMD_ARG];
char  cmdline[BUFSIZ];
int cmdsize;

void fatal(char *str) {
	perror(str);
	exit(1);
}
void zombie_handler(int unused) {
	waitpid(-1, NULL, WNOHANG);
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

void cmd_exit(char *cmd) {
	if (!strcmp(cmd, "exit")) {
		exit(0);
	}
}
void cmd_cd(char **cmd) {
	if (!strcmp(cmd[0], "cd")) {
		chdir(cmd[1]);
	}
}
void cmd_bg(char **cmd) {
	pid_t pid;
	switch (pid = fork()) {
	case 0:
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		setpgid(0, 0);

		cmd[cmdsize - 1] = NULL;
		execvp(cmd[0], cmd);
		fatal("main()");
	case -1:
		fatal("main()");
	default:
		cmd_cd(cmd);
	}
}
int cmd_redir(char **cmd) {
	int i, fd;
	for (i = 0; cmd[i] != NULL; i++) {
		if (!strcmp(cmd[i], ">") || !strcmp(cmd[i], "<") && cmd[i]) {
			if (!cmd[i + 1]) {
				return -1;
			}
			else {
				if (!strcmp(cmd[i], ">")) {
					if ((fd = open(cmd[i + 1], O_RDWR | O_CREAT, 0644)) == -1) {
						return -1;
					}
					else {
						dup2(fd, STDOUT_FILENO);
					}
				}
				else if (!strcmp(cmd[i], "<")) {
					if ((fd = open(cmd[i + 1], O_RDONLY)) == -1) {
						return -1;
					}
					else {
						dup2(fd, STDIN_FILENO);
					}
				}
				close(fd);
				for (i = i; cmd[i] != NULL; i++) {
					cmd[i] = cmd[i + 2];
				}
				for (i = 0; cmd[i] != NULL; i++) {
					if (!strcmp(cmd[i], "<") || !strcmp(cmd[i], ">") && cmd[i + 1]) {
						cmd_redir(cmd);
					}
				}

			}
		}
	}
	return 0;
}
void cmd_pipe(char **cmd) {
	int i;
	pid_t pid1,pid2;
	int fd[2];
	char* arr1[MAX_CMD_ARG];
	char* arr2[MAX_CMD_ARG];
	for (i = 0; cmd[i] != NULL; i++) {
		if (!strcmp(cmd[i], "|")) {
			arr1[i] = NULL;
			break;
		}
		arr1[i] = cmd[i];
	}
	int j = 0;
	for (i = i + 1; cmd[i] != NULL; i++) {
		arr2[j] = cmd[i];
		j++;
	}
	int k;
	for(k=0; arr1[k]!=NULL;k++){
		fprintf(stderr,"%s\n",arr1[k]);
	}
	for(k=0; arr2[k]!=NULL; k++){
		fprintf(stderr,"%s\n",arr2[k]);
	}
	pipe(fd);
	switch (pid1 = fork()) {
	case 0:
		for (i = 0; arr1[i] != NULL; i++) {
			if (!strcmp(arr1[i], "<") || !strcmp(arr1[i], ">")) {
				cmd_redir(arr1);
			}
		}
		dup2(fd[1], STDOUT_FILENO);
		close(fd[1]);
		close(fd[0]);
		execvp(arr1[0], arr1);
	}
	switch (pid2 = fork()) {
	case 0:
		for (i = 0; arr2[i] != NULL; i++) {
			if (!strcmp(arr2[i], "<") || !strcmp(arr2[i], ">")) {
				cmd_redir(arr2);
			}
		}
		dup2(fd[0], STDIN_FILENO);
		close(fd[1]);
		close(fd[0]);
		execvp(arr2[0], arr2);
	}
}

int main(int argc, char**argv) {
	int i = 0;
	pid_t pid;
	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_handler = zombie_handler;
	act.sa_flags = SA_RESTART;
	sigaction(SIGCHLD, &act, NULL);
	signal(SIGSTOP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

	while (1) {
		fputs(prompt, stdout);
		fgets(cmdline, BUFSIZ, stdin);
		cmdline[strlen(cmdline) - 1] = '\0';
		cmdsize = makelist(cmdline, " \t", cmdvector, MAX_CMD_ARG);

		cmd_exit(cmdline);
		if (!strcmp(cmdvector[cmdsize - 1], "&")) {
			cmd_bg(cmdvector);
			continue;
		}
		switch (pid = fork()) {
		case 0:
			signal(SIGINT, SIG_DFL);
			signal(SIGQUIT, SIG_DFL);
			setpgid(0, 0);
			tcsetpgrp(STDIN_FILENO, getpgid(0));
			cmd_pipe(cmdvector);
			cmd_redir(cmdvector);
			execvp(cmdvector[0], cmdvector);
			fatal("main()");
		case -1:
			fatal("main()");
		default:
			waitpid(pid, NULL, 0);
			tcsetpgrp(STDIN_FILENO, getpgid(0));
			cmd_cd(cmdvector);
		}

	}
	return 0;
}


