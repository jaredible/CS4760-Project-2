/*
 * Title: Palindrome Finder (controller)
 * Filename: master.c
 * Usage: ./master -h
 *        ./master [-n x] [-s x] [-t time] infile
 * Author(s): Jared Diehl (jmddnb@umsystem.edu)
 * Date: October 2, 2020
 * Description: Contoller program responsible for creating worker programs to log palindromes.
 * Source(s): https://github.com/jeffcaljr/Unix-Concurrent-Processes-and-Shared-Memory/blob/master/master.cpp
 *                Used to understand program logic.
 *            https://lowlevelbits.org/handling-timeouts-in-child-processes/
 *                Used timeout function.
 *            https://github.com/coreutils/coreutils/blob/master/src/du.c
 *                Used getopt error logic.
 */

#include "shared.h"

/* Function prototypes. */
int loadStrings(char*);
void setupTimer(int);
void spawnChild(int);
void signalHandler(int);

/* Initialize option variables to defaults. */
int n = TOTAL_PROCESSES_MAX_DEFAULT;
int s = CONCURRENT_PROCESSES_DEFAULT;
int t = PROGRAM_DURATION_MAX;

/* Flag for temporarily disabling interrupt handler. */
bool flag = false;

int main(int argc, char** argv) {
	/* Set program name so it can be used globally. */
	programName = argv[0];
	
	/* Set stdout and stderr to unbuffered so that printing is always flushed. */
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	
	/* Register handler for Ctrl+C signal. */
	signal(SIGINT, signalHandler);
	
	/* Clear log files. */
	touchFile("palin.out");
	touchFile("nopalin.out");
	touchFile("output.log");
	
	bool ok = true;
	
	/* Get options from program arguments. */
	while (true) {
		int c = getopt(argc, argv, "hn:s:t:");
		
		if (c == -1) break;
		
		switch (c) {
			case 'h':
				usage(EXIT_SUCCESS);
			case 'n':
				/* Check if "n" argument is not a digit or less than 0. */
				if (!isdigit(*optarg) || (n = atoi(optarg)) < 0) {
					error("invalid max total processes '%s'", optarg);
					ok = false;
				}
				break;
			case 's':
				/* Check if "s" argument is not a digit or less than 0. */
				if (!isdigit(*optarg) || (s = atoi(optarg)) < 0) {
					error("invalid concurrent processes '%s'", optarg);
					ok = false;
				}
				break;
			case 't':
				/* Check if "t" argument is not a digit or less than 0. */
				if (!isdigit(*optarg) || (t = atoi(optarg)) < 0) {
					error("invalid timeout time '%s'", optarg);
					ok = false;
				}
				break;
			default:
				ok = false;
		}
	}
	
	/* Print usage if any option is invalid. */
	if (!ok) usage(EXIT_FAILURE);
	
	/* Check if an input file was supplied. */
	if (argv[optind] == NULL) {
		error("missing input file");
		usage(EXIT_FAILURE);
	}
	
	/* Allocate shared memory. */
	allocateSPM();
	
	/* Load strings and get string count. */
	int c = loadStrings(argv[optind]);
	
	/* Clamp options. */
	n = MIN(c, TOTAL_PROCESSES_MAX);
	s = MIN(s, n);
	t = MIN(t, PROGRAM_DURATION_MAX);
	
	/* Simulate the program exiting instantly. */
	if (n == 0 || s == 0 || t == 0) exit(EXIT_SUCCESS);
	
	/* Set number of workers to ever be in system to shared memory variable. */
	spm->total = n;
	
	/* Set up timer using timeout option variable. */
	setupTimer(t);
	
	/* Initialize variables to help with child-spawning logic. */
	int i = 0;
	int j = n;
	int k = n;
	
	/* Spawn initial children. */
	while (i < s)
		spawnChild(i++);
	
	/* Continue to loop until the total number of processes in system are 0. */
	while (k > 0) {
		/* Wait for a child process to exit. */
		wait(NULL);
		
		/* Log the time it finished. */
		logOutput("output.log", "%s: Process %d finished\n", getFormattedTime(), n - k);
		
		/* Try to spawn another. */
		if (i < j) spawnChild(i++);
		
		k--;
	}
	
	/* Remove shared memory. */
	removeSPM();
	
	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* Loads up to "n" strings from a file given its path "path" into shared memory, and returns number of strings read. */
int loadStrings(char* path) {
	FILE* fp = fopen(path, "r");
	if (fp == NULL) crash("Failed to open file for loading strings");
	
	int i = 0;
	char* line = NULL;
	size_t len = 0;
	ssize_t read;
	
	/* Loop through all lines. */
	while (i < n && (read = getline(&line, &len, fp)) != -1) {
		/* Remove newline from string. */
		removeNewline(line);
		
		/* Copy string into shared memory. */
		strcpy(spm->strings[i++], line);
	}
	
	fclose(fp);
	if (line) free(line);
	
	/* Return number of strings read. */
	return i;
}

/* Sets up timer for timeout functionality. */
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

/* Spawns a child given an index "i". */
void spawnChild(const int i) {
	/* Fork the current process. */
	pid_t pid = fork();
	
	/* Check if fork'ing failed. */
	if (pid == -1) crash("Failed to create a child process for palin");
	
	/* Check if is child process. */
	if (pid == 0) {
		/* Enable flag to slow interrupt handler. */
		flag = true;
		
		if (i == 0) spm->pgid = getpid();
		setpgid(0, spm->pgid);
		
		/* Disable flag to continue interrupt handler. */
		flag = false;
		
		/* Log the time this child process is starting. */
		logOutput("output.log", "%s: Process %d starting\n", getFormattedTime(), i);
		
		/* Convert integer "i" to string "id". */
		char id[256];
		sprintf(id, "%d", i);
		
		/* Execute child process "palin". */
		execl("./palin", "palin", id, (char*) NULL);
		
		/* Exit successfully. */
		exit(EXIT_SUCCESS);
	}
}

/* Responsible for handling Ctrl+C and timeout signals. */
void signalHandler(int s) {
	/* If flag is set, wait for just a bit so the child process has time to set a PGID. */
	if (flag) sleep(1);
	
	/* Initialize a message. */
	char message[4096];
	strfcpy(message, "%s: Exiting due to %s signal\n", getFormattedTime(), s == SIGALRM ? "timeout" : "interrupt");
	
	/* Output that message. */
	fprintf(stderr, message);
	logOutput("output.log", message);
	
	/* Send kill signals to all child processes using appropriate signal. */
	killpg(spm->pgid, s == SIGALRM ? SIGUSR1 : SIGTERM);
	
	/* To avoid having zombie processes, wait for all the children to exit. */
	while (wait(NULL) > 0);
	
	/* Remove shared memory. */
	removeSPM();
	
	/* Exit successfully. */
	exit(EXIT_SUCCESS);
}
