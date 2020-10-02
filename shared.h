#pragma once
#ifndef SHARED_H
#define SHARED_H

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
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define TOTAL_PROCESSES_MAX_DEFAULT 4
#define CONCURRENT_PROCESSES_DEFAULT 2
#define PROGRAM_DURATION_MAX 100
#define TOTAL_PROCESSES_MAX 20
#define STRING_LENGTH_MAX 256
#define CS_SLEEP_MIN 0
#define CS_SLEEP_MAX 2
#define FORMATTED_TIME_SIZE 50
#define FORMATTED_TIME_FORMAT "%H:%M:%S"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

enum state { idle, want_in, in_cs };

struct SharedProgramMemory {
	size_t total;
	enum state flags[TOTAL_PROCESSES_MAX];
	enum state turn;
	char strings[TOTAL_PROCESSES_MAX][STRING_LENGTH_MAX];
	pid_t pgid;
};

char* programName;

int spmKey;
int spmSegmentID;
struct SharedProgramMemory* spm;

void error(char *fmt, ...);
void usage(int);
void touchFile(char*);
void allocateSPM();
void attachSPM();
void releaseSPM();
void deleteSPM();
void removeSPM();
void logOutput(char*, char*, ...);
char* getFormattedTime();
void removeNewline(char*);
void crash(char*, ...);
void strfcat(char*, char*, ...);

#endif
