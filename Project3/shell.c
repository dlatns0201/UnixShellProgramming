#include "shell.h"//

static char* cmdvector[MAX_CMD_ARG];
static char cmdline[BUFSIZ];
static int numtokens;
static int fg_pid;
static int int_pid;
int type[MAX_CMD_ARG];
static char * tmp[MAX_CMD_ARG];

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

void makelist(char *s, const char *delimiters, char** list) {
	int i = 0;
	numtokens = 0;
	char *snew = NULL;
	int flag = 1;
	if ((s == NULL) || (delimiters == NULL)) return;
	snew = s + strspn(s, delimiters);       /* delimiters¸¦ skip */

	while (1) {
		if (flag) {
			flag--;
			list[numtokens] = strtok(snew, delimiters);

		}else{
			list[numtokens] = strtok(NULL, delimiters);
			if(list[numtokens]==NULL)
				return;
		}
		switch (list[numtokens][0]) {
			case '\n':
				type[numtokens]=EOL;
				break;
			case '\0':
				return;
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
		if (numtokens == (MAX_CMD_ARG - 1)) return;
		numtokens++;
	}
	return;
}


static int process_run(char **cmd, int where, int in, int out) { /* Execute a command with optional wait */
	int pid,i;
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
	act.sa_handler = SIG_DFL;
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
		printf("%d process is running at background\n", pid);
		return 0;
	}

	/* Wait until child process exit */
	while (((ret = wait(&status)) != pid) && (ret != -1)) {
		// empty
	}
	fg_pid = 0;
	return (ret == -1) ? 1 : status;
}



int cmd_input(const char *prompt) {
	int cnt = 0;
	int str_len;
	fputs(prompt,stdout);
	fgets(cmdline,BUFSIZ,stdin);

	cmdline[strlen(cmdline)-1]='\0';
	if(!strcmp(cmdline,"exit")){
		return 0;
	}
	return 1;

}

void readyTo_run() {
	int i,j,k;
	int in = -1, out = -1, where;
	int fd[2];
	int flag=0;

	int_pid = 0;
	makelist(cmdline, " \t", cmdvector);
	for (i = 0; i < numtokens; i++) {
		switch (type[i]) {
			case ARG:
				break;
			case LESS:
				if ((in = open(cmdvector[i + 1], O_RDONLY)) == -1) {
					fputs("in error()", stderr);
					return;
				}
				for(j=i;cmdvector[j]!=NULL;j++){
					cmdvector[j]=cmdvector[j+2];
					type[j]=type[j+2];
				}
				i--;
				numtokens-=2;
				break;
			case GREATER:
				if ((out = open(cmdvector[i + 1], O_RDWR | O_CREAT | O_TRUNC, 0666)) == -1) {
					fputs("out error()", stderr);
					return;
				}
				for(j=i;cmdvector[j]!=NULL;j++){
					cmdvector[j]=cmdvector[j+2];
					type[j]=type[j+2];
				}
				numtokens-=2;
				i--;
				break;
			case PIPELINE:
				if (pipe(fd) == -1) {
					fputs("pipe error()",stderr);
					return;
				}
				out = fd[1];
			case AMPERSAND:
				where=(type[i]==AMPERSAND)?BACKGROUND : FOREGROUND;

				if (i != 0) {
					cmdvector[i] = NULL;
					if(flag==0){
						process_run(cmdvector, where, in, out);
					}else{
						process_run(tmp,where,in,out);
					}
					if(where==BACKGROUND)
						return;
					if (in != -1) {
						close(in);
						in = -1;
					}
					if (out != -1) {
						close(out);
						out = -1;
					}

				}
				if (type[i] == PIPELINE){
					flag=1;
					in = fd[0];
					for(j=i+1,k=0;cmdvector[j]!=NULL;j++,k++){
						if(!strcmp(cmdvector[j],"|"))
							break;
						else if(!strcmp(cmdvector[j],"<")){
							i=j-1;
							break;

						}else if(!strcmp(cmdvector[j],">")){
							i=j-1;
							break;
						}else{
							tmp[k]=cmdvector[j];
						}
					}
				}

		}
	}
	if(flag){
		process_run(tmp,where,in,out);
	}else{
		process_run(cmdvector,where,in,out);
	}
	return;

}
