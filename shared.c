#include "shared.h"

void error(char *fmt, ...) {
	int n;
	int size = 100;
	char *p, *np;
	va_list ap;
	
	if ((p = malloc(size)) == NULL) return;
	
	while (true) {
		va_start(ap, fmt);
		n = vsnprintf(p, size, fmt, ap);
		va_end(ap);
		
		if (n < 0) return;
		
		if (n < size) break;
		
		size = n + 1;
		
		if ((np = realloc(p, size)) == NULL) {
			free(p);
			return;
		} else p = np;
	}
	
	fprintf(stderr, "%s: %s\n", programName, p);
}

void usage(int status) {
	if (status != EXIT_SUCCESS) fprintf(stderr, "Try '%s -h' for more information.\n", programName);
	else {
		printf("NAME\n");
		printf("       %s - palindrome finder", programName);
		printf("\nUSAGE\n");
		printf("       %s [-h]\n", programName);
		printf("       %s [-n x] [-s x] [-t time] infile", programName);
		printf("\nDESCRIPTION\n");
		printf("       -h       : Print a help message or usage, and exit\n");
		printf("       -n x     : Maximum total of child processes (default 4)\n");
		printf("       -s x     : Number of children allowed to exist concurrently (default 2)\n");
		printf("       -t time  : Time, in seconds, after which the program will terminate (default 100)\n");
	}
	exit(status);
}

void touchFile(char* path) {
	FILE* fp = fopen(path, "w");
	if (fp == NULL) crash("Failed to touch file");
	fclose(fp);
}

void allocateSPM() {
	logOutput("output.log", "%s: Allocating shared memory\n", getFormattedTime());
	attachSPM();
	releaseSPM();
	attachSPM();
}

void attachSPM() {
	spmKey = ftok("Makefile", 'p');
	if ((spmSegmentID = shmget(spmKey, sizeof(struct SharedProgramMemory), IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) crash("Failed to allocate shared memory for SPM");
	else spm = (struct SharedProgramMemory*) shmat(spmSegmentID, NULL, 0);
}

void releaseSPM() {
	if (spm != NULL) if (shmdt(spm)) crash("Failed to release SPM");
}

void deleteSPM() {
	if (spmSegmentID > 0) if (shmctl(spmSegmentID, IPC_RMID, NULL) < 0) crash("Failed to delete SPM");
}

void removeSPM() {
	logOutput("output.log", "%s: Removing shared memory\n", getFormattedTime());
	releaseSPM();
	deleteSPM();
}

void logOutput(char* path, char* fmt, ...) {
	FILE* fp = fopen(path, "a+");
	
	if (fp == NULL) crash("Failed to open file for logging output");
	
	int n = 4096;
	char buf[n];
	va_list args;
	
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	
	fprintf(fp, buf);
	fclose(fp);
}

char* getFormattedTime() {
	char* formattedTime = malloc(FORMATTED_TIME_SIZE * sizeof(char));
	time_t now = time(NULL);
	strftime(formattedTime, FORMATTED_TIME_SIZE, FORMATTED_TIME_FORMAT, localtime(&now));
	return formattedTime;
}

void removeNewline(char* s) {
	while (*s) {
		if (*s == '\n') *s = '\0';
		s++;
	}
}

void crash(char* fmt, ...) {
	int n = 4096;
	char buf[n];
	va_list args;
	
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	
	char buff[n];
	snprintf(buff, n, "%s: %s", programName, buf);
	
	perror(buff);
	exit(EXIT_FAILURE);
}

void strfcpy(char* src, char* fmt, ...) {
	int n = 4096;
	char buf[n];
	va_list args;
	
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	
	strncpy(src, buf, n);
}
