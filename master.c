/* Title: Palindrome Finder
 * Filename: master.c
 * Usage: ./master -h
 *        ./master [-n x] [-s x] [-t time] infile
 * Author(s): Jared Diehl (jmddnb@umsystem.edu)
 * Date: October 2, 2020
 * Description: TODO
 * Source(s): https://github.com/jeffcaljr/Unix-Concurrent-Processes-and-Shared-Memory/blob/master/master.cpp
 *                TODO */

#include "shared.h"

int loadStrings(char*);
void setupTimer(int);
void spawnChild(int);
void signalHandler(int);

int n = 4;
int s = 2;
int t = PROGRAM_DURATION_MAX;

int main(int argc, char** argv) {
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	
	signal(SIGINT, signalHandler);
	
	remove("palin.out");
	remove("nopalin.out");
	remove("output.log");
	
	while (true) {
		int c = getopt(argc, argv, "hn:s:t:");
		
		if (c == -1) break;
		
		switch (c) {
			case 'h':
				printf("NAME\n");
				printf("       %s - palindrome finder", argv[0]);
				printf("\nUSAGE\n");
				printf("       %s [-h]\n", argv[0]);
				printf("       %s [-n x] [-s x] [-t time] infile", argv[0]);
				printf("\nDESCRIPTION\n");
				printf("       -h       : Print a help message or usage, and exit\n");
				printf("       -n x     : Maximum total of child processes\n");
				printf("       -s x     : Number of children allowed to exist concurrently\n");
				printf("       -t time  : Time, in seconds, after which the program will terminate\n");
				exit(EXIT_SUCCESS);
			case 'n':
				n = atoi(optarg);
				if (n < 0) {
					fprintf(stderr, "Negative arguments are not valid\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 's':
				s = atoi(optarg);
				if (s < 0) {
					fprintf(stderr, "Cannot spawn a negative number of children\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 't':
				t = atoi(optarg);
				if (t < 0) {
					fprintf(stderr, "Master cannot have a run duration of negative time\n");
					exit(EXIT_FAILURE);
				} else if (t == 0) {
					exit(EXIT_SUCCESS);
				}
				break;
			default:
				fprintf(stderr, "Default getopt statement\n");
				exit(EXIT_FAILURE);
		}
	}
	
	allocateSPM();
	
	// TODO
	char* path = "infile";
	if (argv[optind] != NULL) path = argv[optind];
	int c = loadStrings(path);
	n = MIN(c, TOTAL_PROCESSES_MAX);
	s = MIN(s, n);
	t = MIN(t, PROGRAM_DURATION_MAX);
	
	spm->total = n;
	setupTimer(t);
	
	printf("stringCount: %d, n: %d, s: %d, t: %d, total: %d\n", c, n, s, t, (int) spm->total);
	
	int i = 0;
	int j = n;
	int nn = n;
	
	while (i < s)
		spawnChild(i++);
	
	while (nn > 0) {
		wait(NULL);
		logOutput("output.log", "Process %d finished, processes in system %d\n", n - nn, nn);
		if (i < j) spawnChild(i++);
		nn--;
	}
	
	releaseSPM();
	
	return EXIT_SUCCESS;
}

int loadStrings(char* path) {
	FILE* fp = fopen(path, "r");
	if (fp == NULL) crash("fopen: Failed to open file for loading strings");
	
	int i = 0;
	char* line = NULL;
	size_t len = 0;
	ssize_t read;
	while (i < n && (read = getline(&line, &len, fp)) != -1) {
		removeNewline(line);
		strcpy(spm->strings[i++], line);
	}
	
	fclose(fp);
	if (line) free(line);
	
	return i;
}

void setupTimer(const int t) {
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = signalHandler;
	if (sigaction(SIGALRM, &action, NULL) != 0) crash("sigaction");
	
	struct itimerval timer;
	timer.it_value.tv_sec = t;
	timer.it_value.tv_usec = t;
	
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	
	if (setitimer(ITIMER_REAL, &timer, NULL) != 0) crash("setitimer");
}

void spawnChild(const int i) {
	pid_t pid = fork();
	
	if (pid == -1) crash("fork: Failed to create a child process for palin");
	
	if (pid == 0) {
		if (i == 0) spm->pgid = getpid();
		setpgid(0, spm->pgid);
		
		logOutput("output.log", "Process %d starting, processes in system: %d\n", i, i + 1);
		
		char id[256];
		sprintf(id, "%d", i);
		
		execl("./palin", "palin", id, (char*) NULL);
		exit(EXIT_SUCCESS);
	}
}

void signalHandler(int s) {
	printf("master: Exiting due to %s signal\n", s == SIGALRM ? "timeout" : "interrupt");
	killpg(spm->pgid, s == SIGALRM ? SIGUSR1 : SIGTERM);
	int i;
	while (wait(NULL) > 0) {
		i = 1e9;
		while (i-- > 0);
		fprintf(stderr, "wait: Child process took too long to terminate\n");
		break;
	}
	releaseSPM();
	exit(EXIT_SUCCESS);
}
