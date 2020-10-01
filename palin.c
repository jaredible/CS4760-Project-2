#include "shared.h"

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
	
	int index;
	
	if (argc < 2) {
		printf("No arguments supplied");
		exit(1);
	} else {
		i = atoi(argv[1]);
		index = atoi(argv[2]);
	}
	
	srand(time(NULL) + i);
	
	int spmKey = ftok("Makefile", 'p');
	int spmSegmentID;
	struct SharedProcessMemory* spm;
	
	if ((spmSegmentID = shmget(spmKey, sizeof(struct SharedProcessMemory), IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) {
		perror("shmget: Failed to allocate shared memory for SPM");
		exit(1);
	} else {
		spm = (struct SharedProcessMemory*) shmat(spmSegmentID, NULL, 0);
	}
	
	int N = spm->total;
	
	char* string = spm->strings[index];
	bool palindrome = isPalindrome(string);
	
	fprintf(stderr, "%s: Process %d wants to enter critical section\n", getFormattedTime(), i);
	
	int j;
	do {
		spm->flags[i] = want_in;
		j = spm->turn;
		
		while (j != i)
			j = (spm->flags[j] != idle) ? spm->turn : (j + 1) % N;
		
		spm->flags[i] = in_cs;
		
		for (j = 0; j < N; j++)
			if (j != i && spm->flags[j] == in_cs)
				break;
	} while (j < N || (spm->turn != i && spm->flags[spm->turn] != idle));
	
	spm->turn = i;
	
	/* Enter critical section */
	
	fprintf(stderr, "%s: Process %d in critical section\n", getFormattedTime(), i);
	sleep(rand() % 3);
	logPalin(palindrome, string);
	logChild(getFormattedTime(), getpid(), i, string);
	fprintf(stderr, "%s: Process %d exiting critical section\n", getFormattedTime(), i);
	
	/* Exit critical section */
	
	j = (spm->turn + 1) % N;
	while (spm->flags[j] == idle)
		j = (j + 1) % N;
	
	spm->turn = j;
	spm->flags[i] = idle;
	
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
