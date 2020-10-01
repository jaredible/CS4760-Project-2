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

#define TEST 20

void removeLogFiles();
void removeNewline(char*);
int loadStrings(char*);
void spawnChild(int);
void spawn(int);
void executeChild(int, int);
void killSignalHandler(int);
void timeoutSignalHandler(int);
void releaseMemory();

const int MAX_NUM_OF_PROCESSES_IN_SYSTEM = 20;
int currentConcurrentChildCount = 0; // FIXME

int spmKey;
int spmSegmentID;
struct SharedProcessMemory* spm;

int status = 0;

int startTime;

int n = 4;
int s = 2;
int executedChildCount = 0;
int t = TEST;

void setupTimer(int timeout) {
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = timeoutSignalHandler;
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

int main(int argc, char** argv) {
	printf("test: %d\n", test());
	return 0;
	
	setvbuf(stdout, NULL, _IONBF, 0);
	
	signal(SIGINT, killSignalHandler);
	
	spmKey = ftok("Makefile", 'p');
	
	removeLogFiles();
	
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
				exit(0);
			case 'n':
				n = atoi(optarg);
				if (n < 0) {
					fprintf(stderr, "Negative arguments are not valid\n");
					exit(1);
				}
				break;
			case 's':
				s = atoi(optarg);
				if (s < 0) {
					fprintf(stderr, "Cannot spawn a negative number of children\n");
					exit(1);
				}
				break;
			case 't':
				t = atoi(optarg);
				if (t <= 0) {
					fprintf(stderr, "Master can only have a run duration of positive time\n");
					exit(1);
				} else if (t > TEST) {
					t = TEST;
				}
				break;
			default:
				fprintf(stderr, "Default getopt statement\n");
				exit(1);
		}
	}
	
	setupTimer(t * 1000);
	
	if ((spmSegmentID = shmget(spmKey, sizeof(struct SharedProcessMemory), IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
		perror("shmget: Failed to allocate shared memory for SPM");
		exit(1);
	} else {
		spm = (struct SharedProcessMemory*) shmat(spmSegmentID, NULL, 0);
	}
	
	int stringCount = loadStrings(argv[optind]);
	if (n != stringCount) n = stringCount;
	spm->total = stringCount;
	
	int childIndex = 0;
	
	while (childIndex < s)
		spawnChild(childIndex++);

	while (currentConcurrentChildCount > 0) {
		wait(NULL);
		currentConcurrentChildCount--;
//		printf("[finish] %d processes in system\n", currentConcurrentChildCount);
		spawnChild(childIndex++);
	}
	
	releaseMemory();
	
	return 0;
}

void removeLogFiles() {
	remove("palin.out");
	remove("nopalin.out");
	remove("output.log");
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
		strcpy(spm->strings[i++], line);
	}
	
	fclose(fp);
	if (line) free(line);
	
	return i;
}

void spawnChild(int childIndex) {
	bool a = currentConcurrentChildCount < s;
	bool b = executedChildCount < n;
	bool c = executedChildCount < MAX_NUM_OF_PROCESSES_IN_SYSTEM;
	bool canSpawnChild = a && b && c;
	if (canSpawnChild) spawn(childIndex);
}

void spawn(int childIndex) {
	executedChildCount++;
	currentConcurrentChildCount++;
	pid_t pid = fork();
	if (pid == -1) {
		perror("fork: Failed to create a child process for palin");
		exit(1);
	} else if (pid == 0) {
//		printf("[start] %d processes in system.\n", currentConcurrentChildCount);
		if (childIndex == 0) spm->pgid = getpid();
		setpgid(0, spm->pgid);
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
	
	killpg(spm->pgid, SIGTERM);
	
	int status;
	while (wait(&status) > 0) {
//		if (WIFEXITED(status)) printf("OK: Child exited with exit status: %d\n", WEXITSTATUS(status));
//		else printf("ERROR: Child has not terminated correctly\n");
	}
	
	releaseMemory();
	
	exit(0);
}

void timeoutSignalHandler(int signal) {
	printf("master: Exiting due to timeout signal\n");
	
	killpg(spm->pgid, SIGTERM);
	
	int status;
	while (wait(&status) > 0) {
//		if (WIFEXITED(status)) printf("OK: Child exited with exit status: %d\n", WEXITSTATUS(status));
//		else printf("ERROR: Child has not terminated correctly\n");
	}
	
	releaseMemory();
	
	exit(0);
}

void releaseMemory() {
	printf("master: Releasing shared memory\n");
	
	shmdt(spm);
	shmctl(spmSegmentID, IPC_RMID, NULL);
}
