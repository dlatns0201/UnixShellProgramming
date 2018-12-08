#include "shell.h"

static char* cmdvector[MAX_CMD_ARG];
static char  cmdline[BUFSIZ];
static int numtokens;
static int fg_pid;
static int int_pid;

void handler_func(int signo) {
	if (fg_pid == 0) {
		if (signo == SIGINT)
			puts("SIGINT catch");
		else if (signo == SIGQUIT)
			puts("SIGQUIT catch");
		else if (signo == SIGTSTP)
			puts("SIGTSTP catch");
		return;
	}
	if (int_pid == 0) {
		puts("signal again send");
		int_pid++;
	}
	else {
		puts("signalling");
		kill(fg_pid, SIGTERM);
		fg_pid = 0;
	}
}

int *makelist(char *s, const char *delimiters, char** list) {
	int i = 0;
	int err=-1;
	numtokens = 0;
	int type[MAX_CMD_ARG];
	char *snew = NULL;
	int flag = 0;
	if ((s == NULL) || (delimiters == NULL)) return &err;

	snew = s + strspn(s, delimiters);       /* delimiters¸¦ skip */

	while (1) {
		if (flag) {
			flag++;
			list[numtokens] = strtok(snew, delimiters);

		}else
			list[numtokens] = strtok(NULL, delimiters);

		switch (*list[numtokens]) {
		case '\0': break;
		case '&':
			type[numtokens] = AMPERSAND;
			break;
		case '|':
			type[numtokens] = PIPELINE;
			break;
		case '<':
			type[numtokens] = LESS;
			break;
		case '>' :
			type[numtokens] = GREATER;
			break;
		default:
			type[numtokens] = ARG;
		}
		if (numtokens == (MAX_CMD_ARG - 1)) return &err;
		numtokens++;
	}
	return type;
}


static int process_run(char **cmd, int where, int in, int out) { /* Execute a command with optional wait */
	int pid;
	int ret, status;

	/* Implement "cd" command (change directory) */
	if (strcmp(cmd[0], "cd") == 0) {
		if (cmd[2] == NULL) {
			if (chdir(cmd[1]) == -1) {
				fputs("chdir error()",stderr);
				return -1;
			}
		}
		else {
			puts("path error()");
			return -1;
		}
		return 0;
	}

	/* Implement "exit" command (exit the shell) */
	if (strcmp(cmd[0], "exit") == 0) {
		if (cmd[1] == NULL) {
			exit(0);
		}
		else {
			puts("only exit command is needed");
			return -1;
		}
	}

	/* Ignore signal (linux) */
	struct sigaction act;
	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART;

	/* Make new(child) process */
	if ((pid = fork()) < 0) {
		fputs("fork error()", stderr);
		return -1;
	}

	if (pid == 0) { /* Child */
		sigaction(SIGINT, &act, NULL);
		sigaction(SIGQUIT, &act, NULL);
		sigaction(SIGTSTP, &act, NULL);

		/* Change standard input/output if needed */
		if (in != -1) {
			dup2(in, 0);
			close(in);
		}
		if (out != -1) {
			dup2(out, 1);
			close(out);
		}

		/* Run command */
		execvp(*cmd, cmd);

		/* exec error */
		fputs(*cmd, stderr);
		exit(-1);
	}

	/* Parent code */

	fg_pid = pid;

	/* If child is background process, print pid of child and exit */
	if (where == BACKGROUND) {
		fg_pid = 0;
		printf("%d process is running at background", pid);
		return 0;
	}

	/* Wait until child process exit */
	while (((ret = wait(&status)) != pid) && (ret != -1)) {
		// empty
	}

	fg_pid = 0;
	return (ret == -1) ? 1 : status;
}



int cmd_input(char *prompt) {
	int cnt = 0;
	char c;
	printf("%s ", prompt);
	while (1) {
		if ((c = getchar()) == EOF)
			return EOF;
		if (cnt < BUFSIZ)
			cmdline[cnt++] = c;
		if (c == '\n' && cnt == BUFSIZ) {
			printf("Input arg is too long\n");
			printf("%s", prompt);
			cnt = 0;
		}
	}
}

void readyTo_run() {
	int i;
	int in = -1, out = -1, where;
	int fd[2];
	int *type;

	int_pid = 0;
	type=makelist(cmdline, " \t", cmdvector);
	for (i = 0; i < numtokens; i++) {
		where = (type[i] == AMPERSAND) ? BACKGROUND : FOREGROUND;
		switch (type[i]) {
		case ARG:
			break;
		case LESS:
			if ((in = open(cmdvector[i + 1], O_RDONLY)) == -1) {
				fputs("in error()", stderr);
				return;
			}
			break;
		case GREATER:
			if ((out = open(cmdvector[i + 1], O_RDWR | O_CREAT | O_TRUNC, 0666)) == -1) {
				fputs("out error()", stderr);
				return;
			}
			break;
		case PIPELINE:
			if (pipe(fd) == -1) {
				fputs("pipe error()",stderr);
				return;
			}
			out = fd[1];

			if (i != 0) {
				cmdvector[i] = NULL;
				process_run(cmdvector, where, in, out);
				if (in != -1) {
					close(in);
					in = -1;
				}
				if (out != -1) {
					close(out);
					out = -1;
				}

			}
			if (type[i] == PIPELINE)
				in = fd[0];
			break;

		}
	}
}
