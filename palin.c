#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define LINE_COUNT 100
#define STRING_LENGTH 256

enum state { idle, want_in, in_cs };

struct strings {
	char data[LINE_COUNT][STRING_LENGTH];
};

bool isPalindrome(char*);
void logPalin(bool, char*);
void logChild(char*, int, int, char*);
void terminateSignalHandler(int);
void timeoutSignalHandler(int);
char* getFormattedTime();

int i;

int main(int argc, char** argv) {
	setvbuf(stdout, NULL, _IONBF, 0);
	
	signal(SIGTERM, terminateSignalHandler);
//	signal(SIGUSR1, timeoutSignalHandler);
	
	int index;
	
	if (argc < 2) {
		printf("No arguments supplied");
		exit(1);
	} else {
		i = atoi(argv[1]);
		index = atoi(argv[2]);
	}
	
//	printf("index: %d, ppid: %d, pid: %d\n", i, getppid(), getpid());
	
	srand(time(NULL) + i);
	
	int N;
	
	int stringsKey = ftok("Makefile", 1);
	int stringsSegmentID;
	struct strings* strings;
	
	if ((stringsSegmentID = shmget(stringsKey, sizeof(struct strings), IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
		perror("shmget: Failed to allocate shared memory for string array");
		exit(1);
	} else {
		strings = (struct strings*) shmat(stringsSegmentID, NULL, 0);
	}
	
	int childrenKey = ftok("Makefile", 2);
	int childrenSegmentID;
	int* children;
	
	if ((childrenSegmentID = shmget(childrenKey, sizeof(int), IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
		perror("shmget: Failed to allocate to shared memory for child count");
		exit(1);
	} else {
		children = (int*) shmat(childrenSegmentID, NULL, 0);
		N = *children;
	}
	
	int flagsKey = ftok("Makefile", 3);
	int flagsSegmentID;
	int* flags;
	
	if ((flagsSegmentID = shmget(flagsKey, N * sizeof(int), IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
		perror("shmget: Failed to allocate shared memory for flag array");
		exit(1);
	} else {
		flags = (int*) shmat(flagsSegmentID, NULL, 0);
	}
	
	int turnKey = ftok("Makefile", 4);
	int turnSegmentID;
	int* turn;
	
	if ((turnSegmentID = shmget(turnKey, sizeof(int), IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
		perror("shmget: Failed to allocate shared memory for turn");
		exit(1);
	} else {
		turn = (int*) shmat(turnSegmentID, NULL, 0);
	}
	
	char* string = strings->data[index];
	bool palindrome = isPalindrome(string);
//	fprintf(stderr, "%s: Process %d determined that %s is %s\n", getFormattedTime(), i, string, palindrome ? "a palindrome" : "not a palindrome");
	
	fprintf(stderr, "%s: Process %d wants to enter critical section\n", getFormattedTime(), i);
	
	int j;
	do {
		flags[i] = want_in;
		j = *turn;
		
		while (j != i)
			j = (flags[j] != idle) ? *turn : (j + 1) % N;
		
		flags[i] = in_cs;
		
		for (j = 0; j < N; j++)
			if (j != i && flags[j] == in_cs)
				break;
	} while (j < N || (*turn != i && flags[*turn] != idle));
	
	*turn = i;
	
	/* Enter critical section */
	
	fprintf(stderr, "%s: Process %d in critical section\n", getFormattedTime(), i);
	sleep(rand() % 3);
	logPalin(palindrome, string);
	logChild(getFormattedTime(), getpid(), i, string);
	fprintf(stderr, "%s: Process %d exiting critical section\n", getFormattedTime(), i);
	
	/* Exit critical section */
	
	j = (*turn + 1) % N;
	while (flags[j] == idle)
		j = (j + 1) % N;
	
	*turn = j;
	flags[i] = idle;
	
	/* Enter remainder section */
	
	/* Exit remainder section */
	
	return 0;
}

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

void logPalin(bool palindrome, char* string) {
	FILE* fp = fopen(palindrome ? "palin.out" : "nopalin.out", "a+");
	if (fp == NULL) {
		perror("fopen: Failed to open file for palin output");
		exit(1);
	}
	fprintf(fp, "%s\n", string);
	fclose(fp);
}

void logChild(char* time, int pid, int index, char* string) {
	FILE* fp = fopen("output.log", "a+");
	if (fp == NULL) {
		perror("fopen: Failed to open file for child output");
		exit(1);
	}
	fprintf(fp, "%s %d %d %s\n", time, pid, index, string);
	fclose(fp);
}

void terminateSignalHandler(int signal) {
	if (signal == SIGTERM) {
		printf("palin: Process %d exiting due to interrupt signal\n", i);
		exit(1);
	}
}

void timeoutSignalHandler(int signal) {
	if (signal == SIGUSR1) {
		printf("palin: Process %d exiting due to timeout\n", i);
		exit(1);
	}
}

char* getFormattedTime() {
	int n = 100;
	char* result = malloc(n * sizeof(char));
	time_t now = time(0);
	strftime(result, n, "%H:%M:%S", localtime(&now));
	return result;
}
