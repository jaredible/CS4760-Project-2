#include "shared.h"

void allocateSPM() {
	attachSPM();
	releaseSPM();
	attachSPM();
}

void attachSPM() {
	spmKey = ftok("Makefile", 'p');
	if ((spmSegmentID = shmget(spmKey, sizeof(struct SharedProcessMemory), IPC_CREAT | S_IRUSR | S_IWUSR)) < 0) crash("shmget: Failed to allocate shared memory for SPM");
	else spm = (struct SharedProcessMemory*) shmat(spmSegmentID, NULL, 0);
}

void releaseSPM() {
	if (spm != NULL) if (shmdt(spm)) crash("shmdt");
	if (spmSegmentID > 0) if (shmctl(spmSegmentID, IPC_RMID, NULL) < 0) crash("shmctl");
}

void logOutput(char* path, char* fmt, ...) {
	FILE* fp = fopen(path, "a+");
	
	if (fp == NULL) crash("fopen: Failed to open file for logging output");
	
	char buf[4096];
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

void crash(char* message) {
	perror(message);
	exit(EXIT_FAILURE);
}

void strfcat(char* src, char* fmt, ...) {
	char buf[4096];
	va_list args;
	
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	
	strcat(src, buf);
}
