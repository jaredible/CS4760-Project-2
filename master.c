#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *program_name = NULL;

void set_program_name(const char *name);
void usage(int status);

int main(int argc, char **argv) {
	set_program_name(argv[0]);
	
	bool ok = true;
	
	while (true) {
		int c = getopt(argc, argv, "hn:s:t:");
		
		if (c == -1) break;
		
		switch (c) {
			case 'h':
				break;
			case 'n':
				break;
			case 's':
				break;
			case 't':
				break;
			default:
				ok = false;
		}
	}
	
	if (!ok) usage(EXIT_FAILURE);
	
	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

void set_program_name(const char *name) {
	program_name = name;
}

void usage(int status) {
	if (status != EXIT_SUCCESS) fprintf(stderr, "Try '%s -h' for more information.\n", program_name);
	else {
	}
}
