#include <ctype.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_TOTAL_CHILD_COUNT 4
#define DEFAULT_CONCURRENT_CHILD_COUNT 2
#define DEFAULT_TERMINATION_TIMEOUT 100

const char *program_name = NULL;

void set_program_name(const char *name);
void error(const char *fmt, ...);
void usage(int status);

int main(int argc, char **argv) {
	set_program_name(argv[0]);
	
	bool ok = true;
	int total_child_count = DEFAULT_TOTAL_CHILD_COUNT;
	int concurrent_child_count = DEFAULT_CONCURRENT_CHILD_COUNT;
	int termination_timeout = DEFAULT_TERMINATION_TIMEOUT;
	
	while (true) {
		int c = getopt(argc, argv, "hn:s:t:");
		
		if (c == -1) break;
		
		switch (c) {
			case 'h':
				usage(EXIT_SUCCESS);
			case 'n':
				if (isdigit(*optarg)) {
					total_child_count = atoi(optarg);
				} else {
					error("invalid total child count '%s'", optarg);
					ok = false;
				}
				break;
			case 's':
				if (isdigit(*optarg)) {
					concurrent_child_count = atoi(optarg);
				} else {
					error("invalid concurrent child count '%s'", optarg);
					ok = false;
				}
				break;
			case 't':
				if (isdigit(*optarg)) {
					termination_timeout = atoi(optarg);
				} else {
					error("invalid termination timeout '%s'", optarg);
					ok = false;
				}
				break;
			default:
				ok = false;
		}
	}
	
	if (argv[optind] == NULL || argc > optind + 1) {
		error("invalid arguments specified");
		ok = false;
	}
	
	if (!ok) usage(EXIT_FAILURE);
	
	// input file specified
	
	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

void set_program_name(const char *name) {
	program_name = name;
}

void error(const char *fmt, ...) {
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
	
	fprintf(stderr, "%s: %s\n", program_name, p);
}

void usage(int status) {
	if (status != EXIT_SUCCESS) fprintf(stderr, "Try '%s -h' for more information.\n", program_name);
	else {
		printf("NAME\n");
		printf("       %s - palindrome finder");
		printf("\nUSAGE\n");
		printf("       %s [-h]\n", program_name);
		printf("       %s [-n x] [-s x] [-t time] infile", program_name);
		printf("\nDESCRIPTION\n");
		printf("       -h       : Print a help message or usage, and exit\n");
		printf("       -n x     : Maximum total of child processes\n");
		printf("       -s x     : Number of children allowed to exist concurrently\n");
		printf("       -t time  : Time, in seconds, after which the program will terminate\n");
	}
	exit(status);
}
