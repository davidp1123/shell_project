#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#define MAX_CMD_ARG 10
#define BUFSIZ 256

const char *prompt = "myshell> ";
char *cmdvector[MAX_CMD_ARG];
char cmdline[BUFSIZ], cmdline1[BUFSIZ];
char *filename, *cmd1, *cmd2;
pid_t pid, ppid;
int dupbackup[2], split[100], cnt;
int pipepos, ispipe;

void fatal(char *str)
{
	perror(str);
	exit(1);
}
int makelist(char *s, const char *delimiters, char **list, int MAX_LIST)
{
	int numtokens = 0;
	char *snew = NULL;

	if ((s == NULL) || (delimiters == NULL))
		return -1;

	snew = s + strspn(s, delimiters); /* Skip delimiters */
	if ((list[numtokens] = strtok(snew, delimiters)) == NULL)
		return numtokens;

	numtokens = 1;

	while (1)
	{
		if ((list[numtokens] = strtok(NULL, delimiters)) == NULL)
			break;
		if (numtokens == (MAX_LIST - 1))
			return -1;
		numtokens++;
	}
	return numtokens;
}
void handle1(int unused)
{
	printf("\n");
}
void redirectionCheck(char **vector, int argcnt)
{
	int fd;
	for (int i = 0; i < argcnt; i++)
	{
		if (vector[i][0] == '<')
		{
			vector[i] = '\0';
			filename = vector[i + 1];
			fd = open(filename, O_RDONLY);
			dup2(fd, 0);
		}
		else if (vector[i][0] == '>')
		{
			vector[i] = '\0';
			filename = vector[i + 1];
			fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			dup2(fd, 1);
		}
	}
}
void pipeCheck(char **vector, int argcnt)
{
	//ispipe = 0;
	int pipefd[2], status;
	for (int i = 0; i < argcnt; i++)
	{
		if (vector[i][0] == '|')
		{
			ispipe = 1;
			cmd1 = strtok(cmdline1, "|");
			cmd1 = strtok(NULL, "|");
			cmd2 = strtok(cmdline1, "|");
			break;
		}
	}
	if (ispipe == 1)
	{
		switch (fork())
		{
		case 0:
			break;
		default:
			wait(&status);
			return (status);
		}
		pipe(pipefd);
		switch (fork())
		{
		case 0:
			dup2(pipefd[1], 1);
			close(pipefd[0]);
			close(pipefd[1]);
			execvp(cmd2[0], cmd2);
			fatal("pipe");
		default:
			dup2(pipefd[0], 0);
			close(pipefd[0]);
			close(pipefd[1]);
			execvp(cmd1[0], cmd1);
			fatal("pipe");
		}
	}
}
int main(int argc, char **argv)
{
	int pipePos = -1;
	int i = 0;
	int argcnt;
	int background = 0;
	ppid = getpid();

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	dupbackup[0] = dup(0);
	dupbackup[1] = dup(1);

	while (1)
	{
		dup2(dupbackup[0], 0);
		dup2(dupbackup[1], 1);

		fputs(prompt, stdout);
		if (fgets(cmdline, BUFSIZ, stdin) == NULL)
			continue;

		strcpy(cmdline1, cmdline);

		background = 0;
		ispipe = 0;

		if (strlen(cmdline) > 1)
			cmdline[strlen(cmdline) - 1] = '\0';

		for (i = 0; i < 255; i++)
		{
			if (cmdline[i] == '&')
			{
				cmdline[i] = '\0';
				background = 1;
				break;
			}
		}
		argcnt = makelist(cmdline, " \t", cmdvector, MAX_CMD_ARG);

		pipeCheck(cmdvector, argcnt);
		redirectionCheck(cmdvector, argcnt);

		if (strlen(cmdline) < 2)
			continue;

		if (strcmp(cmdvector[0], "cd") == 0)
			chdir(cmdvector[1]);
		else if (strcmp(cmdvector[0], "exit") == 0)
			exit(0);
		else
		{
			if(ispipe == 0)
			{
				switch (pid = fork())
				{
				case 0:
					signal(SIGINT, SIG_DFL);
					signal(SIGQUIT, SIG_DFL);
					execvp(cmdvector[0], cmdvector); //ignore만 상속을 받고 나머지는 default가 된다
					fatal("main()");
				case -1:
					fatal("main()");
				default:
					if (background == 0)
						waitpid(pid, NULL, NULL);
				}
			}
		}
	}
	return 0;
}