/*
 * Title:
 * Filename: shared.h
 * Usage: N/A
 * Author(s): Jared Diehl (jmddnb@umsystem.edu)
 * Date: October 2, 2020
 * Description: Header file for shared resources.
 * Source(s):
 */

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

/* Define constants. */
#define TOTAL_PROCESSES_MAX_DEFAULT 4
#define CONCURRENT_PROCESSES_DEFAULT 2
#define PROGRAM_DURATION_MAX 100
#define TOTAL_PROCESSES_MAX 20
#define STRING_LENGTH_MAX 256
#define CS_SLEEP_MIN 0
#define CS_SLEEP_MAX 2
#define FORMATTED_TIME_SIZE 50
#define FORMATTED_TIME_FORMAT "%H:%M:%S"

/* Define macros. */
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/* Used in mutual exclusion algorithm. */
enum state { idle, want_in, in_cs };

/* Data structure that's used to communicate between all processes. */
struct SharedProgramMemory {
	size_t total;
	enum state flags[TOTAL_PROCESSES_MAX];
	enum state turn;
	char strings[TOTAL_PROCESSES_MAX][STRING_LENGTH_MAX];
	pid_t pgid;
};

/* Name of current program executing. */
char* programName;

/* Shared memory variables. */
int spmKey;
int spmSegmentID;
struct SharedProgramMemory* spm;

/* Function prototypes. */
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
void strfcpy(char*, char*, ...);

#endif
