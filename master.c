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

int n = TOTAL_PROCESSES_MAX_DEFAULT;
int s = CONCURRENT_PROCESSES_DEFAULT;
int t = PROGRAM_DURATION_MAX;

int main(int argc, char** argv) {
	programName = argv[0];
	
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	
	signal(SIGINT, signalHandler);
	
	touchFile("palin.out");
	touchFile("nopalin.out");
	touchFile("output.log");
	
	bool ok = true;
	
	while (true) {
		int c = getopt(argc, argv, "hn:s:t:");
		
		if (c == -1) break;
		
		switch (c) {
			case 'h':
				usage(EXIT_SUCCESS);
			case 'n':
				if (!isdigit(*optarg) || (n = atoi(optarg)) < 0) {
					error("invalid max total processes '%s'", optarg);
					ok = false;
				}
				break;
			case 's':
				if (!isdigit(*optarg) || (s = atoi(optarg)) < 0) {
					error("invalid concurrent processes '%s'", optarg);
					ok = false;
				}
				break;
			case 't':
				if (!isdigit(*optarg) || (t = atoi(optarg)) < 0) {
					error("invalid timeout time '%s'", optarg);
					ok = false;
				}
				break;
			default:
				ok = false;
		}
	}
	
	if (!ok) usage(EXIT_FAILURE);
	
	if (argv[optind] == NULL) {
		error("missing input file");
		usage(EXIT_FAILURE);
	}
	
	if (t == 0) exit(EXIT_SUCCESS);
	
	allocateSPM();
	
	int c = loadStrings(argv[optind]);
	n = MIN(c, TOTAL_PROCESSES_MAX);
	s = MIN(s, n);
	t = MIN(t, PROGRAM_DURATION_MAX);
	
	spm->total = n;
	setupTimer(t);
	
	int i = 0;
	int j = n;
	int nn = n;
	
	while (i < s)
		spawnChild(i++);
	
	while (nn > 0) {
		wait(NULL);
		logOutput("output.log", "%s: Process %d finished\n", getFormattedTime(), n - nn);
		if (i < j) spawnChild(i++);
		nn--;
	}
	
	removeSPM();
	
	return EXIT_SUCCESS;
}

int loadStrings(char* path) {
	FILE* fp = fopen(path, "r");
	if (fp == NULL) crash("Failed to open file for loading strings");
	
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
	if (sigaction(SIGALRM, &action, NULL) != 0) crash("Failed to set signal action for timer");
	
	struct itimerval timer;
	timer.it_value.tv_sec = t;
	timer.it_value.tv_usec = t;
	
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	
	if (setitimer(ITIMER_REAL, &timer, NULL) != 0) crash("Failed to set timer");
}

void spawnChild(const int i) {
	pid_t pid = fork();
	
	if (pid == -1) crash("Failed to create a child process for palin");
	
	if (pid == 0) {
		if (i == 0) spm->pgid = getpid();
		setpgid(0, spm->pgid);
		
		logOutput("output.log", "%s: Process %d starting\n", getFormattedTime(), i);
		
		char id[256];
		sprintf(id, "%d", i);
		
		execl("./palin", "palin", id, (char*) NULL);
		exit(EXIT_SUCCESS);
	}
}

void signalHandler(int s) {
	char message[4096];
	strfcat(message, "%s: Exiting due to %s signal\n", getFormattedTime(), s == SIGALRM ? "timeout" : "interrupt");
	fprintf(stderr, message);
	logOutput("output.log", message);
	killpg(spm->pgid, s == SIGALRM ? SIGUSR1 : SIGTERM);
	while (wait(NULL) > 0);
	removeSPM();
	exit(EXIT_SUCCESS);
}
