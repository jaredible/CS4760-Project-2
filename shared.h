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

#define TOTAL_PROCESSES_MAX 20
#define STRING_LENGTH_MAX 256

enum state { idle, want_in, in_cs };

struct SharedProcessMemory {
	size_t total;
	enum state flags[TOTAL_PROCESSES_MAX];
	enum state turn;
	char strings[TOTAL_PROCESSES_MAX][STRING_LENGTH_MAX];
	pid_t pgid;
};

int test();

#endif
