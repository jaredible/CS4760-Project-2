/*
 * Title: Palindrome Finder (worker)
 * Filename: palin.c
 * Usage: ./palin index
 * Author(s): Jared Diehl (jmddnb@umsystem.edu)
 * Date: October 2, 2020
 * Description: Worker program responsible for logging if a string is a palindrome.
 * Source(s): https://github.com/jeffcaljr/Unix-Concurrent-Processes-and-Shared-Memory/blob/master/slave.cpp
 *                Used to understand program logic.
 *            https://github.com/sherdwhite/UMSL-Operating-Systems/blob/master/o2-white.2/palin.c
 *                Used palindrome-checking logic.
 *            http://www.cs.umsl.edu/~sanjiv/classes/cs4760/lectures/ipc.pdf
 *                Used mutual exclusion algorithm (solution 4).
 */

#include "shared.h"

/* Function prototypes. */
void process(const int);
bool isPalindrome(char*);
void signalHandler(int);

/* Stores index of string to be checked if palindrome. */
int id;

int main(int argc, char** argv) {
	/* Set program name so it can be used globally. */
	programName = argv[0];
	
	/* Set stdout and stderr to unbuffered so that printing is always flushed. */
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	
	/* Register handlers for Ctrl+C and timeout signals. */
	signal(SIGTERM, signalHandler);
	signal(SIGUSR1, signalHandler);
	
	/* Check and assign arguments. */
	if (argc < 2) crash("No argument supplied for id");
	else id = atoi(argv[1]);
	
	/* Set random's seed using time with an offset. */
	srand(time(NULL) + id);
	
	/* Attach to shared memory. */
	attachSPM();
	
	/* Process string to be checked if palindrome and write to appropriate file. */
	process(id);
	
	return EXIT_SUCCESS;
}

/* Processes string to checked, at index "i", and writes it with additional information to appropriate file. */
void process(const int i) {
	/* Get the total number of workers ever to be in system. */
	int n = spm->total;
	
	/* Get the string to check using index "i". */
	char* string = spm->strings[i];
	
	/* Get if the string is a palindrome. */
	bool palindrome = isPalindrome(string);
	
	/* Output that process wants to enter critical section. */
	fprintf(stderr, "%s: Process %d wants to enter critical section\n", getFormattedTime(), i);
	
	int j;
	do {
		spm->flags[i] = want_in;
		j = spm->turn;
		
		while (j != i)
			j = (spm->flags[j] != idle) ? spm->turn : (j + 1) % n;
		
		spm->flags[i] = in_cs;
		
		for (j = 0; j < n; j++)
			if (j != i && spm->flags[j] == in_cs) break;
	} while (j < n || (spm->turn != i && spm->flags[spm->turn] != idle));
	
	spm->turn = i;
	
	/* Enter critical section */
	
	/* Output that process is in critical section. */
	fprintf(stderr, "%s: Process %d in critical section\n", getFormattedTime(), i);
	
	/* Sleep for at most 2 seconds. */
	sleep(rand() % (CS_SLEEP_MAX - CS_SLEEP_MIN + 1) + CS_SLEEP_MIN);
	
	/* Log time, PID, index, and string to appropriate output file. */
	logOutput(palindrome ? "palin.out" : "nopalin.out", "%s %d %d %s\n", getFormattedTime(), getpid(), i, string);
	
	/* Output that process is exiting critical section. */
	fprintf(stderr, "%s: Process %d exiting critical section\n", getFormattedTime(), i);
	
	/* Exit critical section */
	
	j = (spm->turn + 1) % n;
	while (spm->flags[j] == idle)
		j = (j + 1) % n;
	
	spm->turn = j;
	spm->flags[i] = idle;
	
	/* Enter remainder section */
	/* Exit remainder section */
}

/* Returns if "string" is a palindrome. */
bool isPalindrome(char* string) {
	int leftIndex = 0, rightIndex = strlen(string) - 1;
	char leftChar, rightChar;
	
	while (rightIndex > leftIndex) {
		leftChar = tolower(string[leftIndex]);
		rightChar = tolower(string[rightIndex]);
		
		if (leftChar != rightChar) return false;
		
		leftIndex++;
		rightIndex--;
	}
	
	return true;
}

/* Responsible for handling Ctrl+C and timeout signals. */
void signalHandler(int s) {
	if (s == SIGTERM || s == SIGUSR1) {
		/* Initialize a message. */
		char message[4096];
		strfcpy(message, "%s: Process %d exiting due to %s signal\n", getFormattedTime(), id, s == SIGUSR1 ? "timeout" : "interrupt");
		
		/* Output that message. */
		fprintf(stderr, message);
		logOutput("output.log", message);
		
		/* Exit abnormally. */
		exit(EXIT_FAILURE);
	}
}
