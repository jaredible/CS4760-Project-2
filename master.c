#include <ctype.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#define LINE_COUNT 100
#define STRING_LENGTH 256

struct strings {
	char data[LINE_COUNT][STRING_LENGTH];
};

void clearLogFiles();
void removeNewline(char*);
int loadStrings(char*);
void spawnChild(int);
void spawn(int);
void executeChild(int, int);
void killSignalHandler(int);
void timerSignalHandler(int);
void releaseMemory();

const int MAX_NUM_OF_PROCESSES_IN_SYSTEM = 20;
int currentConcurrentChildCount = 0; // FIXME

int stringsKey;
int stringsSegmentID;
struct strings* strings;

int childrenKey;
int childrenSegmentID;
int* children;

int flagsKey;
int flagsSegmentID;
int *flags;

int turnKey;
int turnSegmentID;
int *turn;

int childProcessGroupKey;
int childProcessGroupSegmentID;
pid_t* childProcessGroup;

int status = 0;

int startTime;

int maximumChildCount = 4;
int concurrentChildCount = 2;
int executedChildCount = 0;
int durationBeforeTermination = 5;

void setupTimer(int timeout) {
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = killSignalHandler;
	if (sigaction(SIGALRM, &action, NULL) != 0) {
		perror("sigaction");
		abort();
	}
	
	struct itimerval timer;
	timer.it_value.tv_sec = timeout / 1000;
	timer.it_value.tv_usec = (timeout % 1000) * 1000;
	
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	
	if (setitimer(ITIMER_REAL, &timer, NULL) != 0) {
		perror("setitimer");
		abort();
	}
}

int main(int argc, char **argv) {
	setvbuf(stdout, NULL, _IONBF, 0);
	
	setupTimer(10 * 1000);
	signal(SIGINT, killSignalHandler);
//	signal(SIGUSR2, timerSignalHandler);
	
	stringsKey = ftok("Makefile", 1);
	childrenKey = ftok("Makefile", 2);
	flagsKey = ftok("Makefile", 3);
	turnKey = ftok("Makefile", 4);
	childProcessGroupKey = ftok("Makefile", 5);
	
	clearLogFiles();
	
	while (true) {
		int c = getopt(argc, argv, "hn:s:t:");
		
		if (c == -1) break;
		
		switch (c) {
			case 'h':
				printf("NAME\n");
				printf("       %s - palindrome finder");
				printf("\nUSAGE\n");
				printf("       %s [-h]\n", argv[0]);
				printf("       %s [-n x] [-s x] [-t time] infile", argv[0]);
				printf("\nDESCRIPTION\n");
				printf("       -h       : Print a help message or usage, and exit\n");
				printf("       -n x     : Maximum total of child processes\n");
				printf("       -s x     : Number of children allowed to exist concurrently\n");
				printf("       -t time  : Time, in seconds, after which the program will terminate\n");
				exit(0);
			case 'n':
				maximumChildCount = atoi(optarg);
				if (maximumChildCount < 0) {
					fprintf(stderr, "Negative arguments are not valid\n");
					exit(1);
				}
				break;
			case 's':
				concurrentChildCount = atoi(optarg);
				if (concurrentChildCount < 0) {
					fprintf(stderr, "Cannot spawn a negative number of children\n");
					exit(1);
				}
				break;
			case 't':
				durationBeforeTermination = atoi(optarg);
				if (durationBeforeTermination < 0) {
					fprintf(stderr, "Master cannot have a run duration of negative time\n");
					exit(1);
				}
				break;
			default:
				fprintf(stderr, "Default getopt statement\n");
				exit(1);
		}
	}
	
	int stringCount;
	
	if ((stringsSegmentID = shmget(stringsKey, sizeof(struct strings), IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
		perror("shmget: Failed to allocate shared memory for string array");
		exit(1);
	} else {
		strings = (struct strings*) shmat(stringsSegmentID, NULL, 0);
		stringCount = loadStrings(argv[optind]);
		if (maximumChildCount != stringCount) maximumChildCount = stringCount;
	}
	
	if ((childrenSegmentID = shmget(childrenKey, sizeof(int), IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
		perror("shmget: Failed to allocate shared memory for child count");
		exit(1);
	} else {
		children = (int*) shmat(childrenSegmentID, NULL, 0);
		*children = stringCount;
	}
	
	if ((flagsSegmentID = shmget(flagsKey, *children * sizeof(int), IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
		perror("shmget: Failed to allocate shared memory for flag array");
		exit(1);
	} else {
		flags = (int*) shmat(flagsSegmentID, NULL, 0);
	}
	
	if ((turnSegmentID = shmget(turnKey, sizeof(int), IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
		perror("shmget: Failed to allocate shared memory for turn");
		exit(1);
	} else {
		turn = (int*) shmat(turnSegmentID, NULL, 0);
	}
	
	if ((childProcessGroupSegmentID = shmget(childProcessGroupKey, sizeof(pid_t), IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
		perror("shmget: Failed to allocate shared memory for group PID");
		exit(1);
	} else {
		childProcessGroup = (pid_t*) shmat(childProcessGroupSegmentID, NULL, 0);
	}
	
	startTime = time(0);
	
	int childIndex = 0;
	
	while (childIndex < concurrentChildCount)
		spawnChild(childIndex++);

	while(currentConcurrentChildCount > 0) {
		wait(NULL);
		currentConcurrentChildCount--;
		printf("[finish] %d processes in system\n", currentConcurrentChildCount);
		spawnChild(childIndex++);
	}
	
	releaseMemory();
	
	return 0;
}

void clearLogFiles() {
	char* files[] = {"palin.out", "nopalin.out", "output.log"};
	int i = 0;
	for (; i < 3; i++) {
		FILE* fp = fopen(files[i], "w");
		fclose(fp);
	}
}

void removeNewline(char* s) {
	while (*s) {
		if (*s == '\n') *s = '\0';
		s++;
	}
}

int loadStrings(char* path) {
	FILE* fp = fopen(path, "r");
	if (fp == NULL) {
		perror("fopen: Failed to open file for loading strings");
		exit(1);
	}
	
	int i = 0;
	char* line = NULL;
	size_t len = 0;
	ssize_t read;
	while ((read = getline(&line, &len, fp)) != -1) {
		removeNewline(line);
		strcpy(strings->data[i++], line);
	}
	
	fclose(fp);
	if (line) free(line);
	
	return i;
}

void spawnChild(int childIndex) {
	bool canSpawnChild = currentConcurrentChildCount < concurrentChildCount && executedChildCount < maximumChildCount && executedChildCount < MAX_NUM_OF_PROCESSES_IN_SYSTEM;
	if (canSpawnChild) {
		spawn(childIndex);
	} else {
//		printf("HERE %d\n", childIndex);
//		waitpid(-(*childProcessGroup), &status, 0);
//		currentConcurrentChildCount--;
//		printf("[idk] %d processes in system.\n", currentConcurrentChildCount);
//		spawn(childIndex);
	}
	executedChildCount++;
}

void spawn(int childIndex) {
	currentConcurrentChildCount++;
	if (fork() == 0) {
		printf("[start] %d processes in system.\n", currentConcurrentChildCount);
		if (childIndex == 0) *childProcessGroup = getpid();
		setpgid(0, *childProcessGroup);
		printf("[spawn] ppid: %d, pid: %d, childProcessGroup: %d\n", getppid(), getpid(), *childProcessGroup);
		executeChild(childIndex, childIndex);
		exit(0);
	}
}

void executeChild(int childIndex, int stringIndex) {
	char ci[256];
	sprintf(ci, "%d", childIndex);
	
	char si[256];
	sprintf(si, "%d", stringIndex);
	
	execl("./palin", "palin", ci, si, (char*) NULL);
}

void killSignalHandler(int signal) {
	printf("master: Exiting due to interrupt signal\n");
	
	killpg(*childProcessGroup, SIGTERM);
	
	int i = 0;
	for (; i < currentConcurrentChildCount; i++)
		wait(NULL);
	
	releaseMemory();
	
	exit(0);
}

void timerSignalHandler(int signal) {
	if (time(0) - startTime >= durationBeforeTermination) {
		printf("master: Timed out\n");
		
		killpg(*childProcessGroup, SIGUSR1);
		
		int i = 0;
		for (; i < currentConcurrentChildCount; i++)
			wait(NULL);
		
		releaseMemory();
		
		exit(0);
	}
}

void releaseMemory() {
	printf("master: Releasing shared memory\n");
	
	shmdt(strings);
	shmctl(stringsSegmentID, IPC_RMID, NULL);
	
	shmdt(children);
	shmctl(childrenSegmentID, IPC_RMID, NULL);
	
	shmdt(flags);
	shmctl(flagsSegmentID, IPC_RMID, NULL);
	
	shmdt(turn);
	shmctl(turnSegmentID, IPC_RMID, NULL);
	
	shmdt(childProcessGroup);
	shmctl(childProcessGroupSegmentID, IPC_RMID, NULL);
}
