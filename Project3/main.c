#include "shell.h"

static const char *prompt = "myshell> ";

int main() {
	struct sigaction act;
	act.sa_handler = handler_func;
	sigfillset(&act.sa_mask);
	act.sa_flags = SA_RESTART;

	sigaction(SIGINT, &act, NULL);
	sigaction(SIGQUIT, &act, NULL);
	sigaction(SIGTSTP, &act, NULL);

	while (cmd_input(prompt) != EOF) {
		readyTo_run();
	}
	return 0;
}