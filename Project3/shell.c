#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>

#define MAX_CMD_ARG 10

void cmd_pipe_exec(char **cmd);
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
		cmd_pipe_exec(cmdvector);
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
					if ((fd = open(cmd[i + 1], O_RDWR | O_CREAT | O_TRUNC, 0644)) == -1) {
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

			}
		}
	}
	return 0;
}
void join(char **cmd1, char **cmd2) {
	int p[2];
	pid_t pid;
	switch (pid = fork()) {
		case 0:
			break;
		default:
			wait(NULL);
			return;
	}
	pipe(p);
	switch (pid = fork()) {
		case 0:
			dup2(p[1],STDOUT_FILENO);
			close(p[0]);
			close(p[1]);
			execvp(cmd1[0], cmd1);
		default:
			dup2(p[0], STDIN_FILENO);
			close(p[0]);
			close(p[1]);
			execvp(cmd2[0], cmd2);
	}
}
void join2(char **cmd, char **cmd1, char **cmd2) {
	int p1[2], p2[2];
	pid_t pid, pid1, pid2;
	switch (pid = fork()) {
		case 0:
			break;
		default:
			waitpid(pid, NULL, 0);
			return;
	}
	pipe(p1);
	switch (pid1 = fork()) {
		case 0:
			break;
		default:
			waitpid(pid1, NULL, 0);
			dup2(p1[0], STDIN_FILENO);
			close(p1[0]);
			close(p1[1]);
			execvp(cmd2[0], cmd2);
	}
	pipe(p2);
	switch (pid2 = fork()) {
		case 0:
			dup2(p2[1], STDOUT_FILENO);
			close(p2[1]);	close(p2[0]);
			close(p1[1]);	close(p1[0]);
			execvp(cmd[0], cmd);
		default:
			waitpid(pid2, NULL, 0);
			dup2(p2[0], STDIN_FILENO);
			dup2(p1[1], STDOUT_FILENO);
			close(p2[0]);	close(p2[1]);
			close(p1[0]);	close(p1[0]);
			execvp(cmd1[0], cmd1);
	}
	return;
}
void cmd_pipe1(char **cmd, int n) {
	int i, j;
	char *cmd_buf[MAX_CMD_ARG];
	for (i = n + 1, j = 0; cmd[i] != NULL; i++, j++) {
		cmd_buf[j] = cmd[i];
		if (cmd[i + 1] == NULL) {
			cmd_buf[i] = NULL;
		}
	}
	for (i = n; cmd[i] != NULL; i++) {
		cmd[i] = NULL;
	}
	join(cmd, cmd_buf);
	exit(0);
}
void cmd_pipe2(char **cmd, int n, int m) {
	int i, j;
	char *cmd_buf1[MAX_CMD_ARG];
	char *cmd_buf2[MAX_CMD_ARG];
	for (i = n + 1, j = 0; i < m; i++, j++) {
		cmd_buf1[j] = cmd[i];
		if (i == m - 1) {
			cmd_buf1[i] = NULL;
		}
	}
	for (i = m + 1, j = 0; cmd[i] != NULL; i++, j++) {
		cmd_buf2[j] = cmd[i];
		if (cmd[i + 1] == NULL) {
			cmd_buf2[i] = NULL;
		}
	}
	for (i = n; cmd[i] != NULL; i++) {
		cmd[i] = NULL;
	}
	join2(cmd, cmd_buf1, cmd_buf2);
	exit(0);
}
void cmd_pipe_exec(char **cmd) {
	int i,fd;
	int flag=0;
	pid_t pid;
	int cnt = 0;
	int cnt1_i, cnt2_i;
	for (i = 0; cmd[i] != NULL; i++) {
		if (!strcmp(cmd[i], ">") || !strcmp(cmd[i], "<") && cmd[i]) {
			cmd_redir(cmd);	
			flag=1;
			switch(pid=fork()){
				case 0:
					cmd_redir(cmd);
					execvp(cmd[0],cmd);
				default:
					waitpid(pid,NULL,0);
			}
		}

		if (!strcmp(cmd[i], "|")) {
			cnt++;
			if (cnt == 1)
				cnt1_i = i;
			if (cnt == 2)
				cnt2_i = i;
		}
	}
	if (cnt == 1) {
		cmd_pipe1(cmd, cnt1_i);
		exit(0);
	}
	else if (cnt == 2) {
		cmd_pipe2(cmd, cnt1_i, cnt2_i);
		exit(0);
	}else if(flag==1){
		exit(0);
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
				cmd_pipe_exec(cmdvector);
					
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
