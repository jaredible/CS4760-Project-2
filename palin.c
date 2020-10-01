#include "shared.h"

void process(const int);
bool isPalindrome(char*);
void signalHandler(int);

int id;

int main(int argc, char** argv) {
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	
	signal(SIGTERM, signalHandler);
	signal(SIGUSR1, signalHandler);
	
	if (argc < 2) crash("No arguments supplied");
	else id = atoi(argv[1]);
	
	srand(time(NULL) + id);
	
	attachSPM();
	
	process(id);
	
	return EXIT_FAILURE;
}

void process(const int i) {
	int n = spm->total;
	
	char* string = spm->strings[i];
	bool palindrome = isPalindrome(string);
	
	fprintf(stderr, "%s: Process %d wants to enter critical section\n", getFormattedTime(), i);
	
	int j;
	do {
		spm->flags[i] = want_in;
		j = spm->turn;
		
		while (j != i)
			j = (spm->flags[j] != idle) ? spm->turn : (j + 1) % n;
		
		spm->flags[i] = in_cs;
		
		for (j = 0; j < n; j++)
			if (j != i && spm->flags[j] == in_cs) break;
	} while (j < n || (spm->turn != i && spm->flags[spm->turn] != idle));
	
	spm->turn = i;
	
	/* Enter critical section */
	
	fprintf(stderr, "%s: Process %d in critical section\n", getFormattedTime(), i);
	sleep(rand() % (CS_SLEEP_MAX - CS_SLEEP_MIN + 1) + CS_SLEEP_MIN);
	logOutput(palindrome ? "palin.out" : "nopalin.out", "%s\n", string);
	logOutput("output.log", "%s %d %d %s\n", getFormattedTime(), getpid(), i, string);
	fprintf(stderr, "%s: Process %d exiting critical section\n", getFormattedTime(), i);
	
	/* Exit critical section */
	
	j = (spm->turn + 1) % n;
	while (spm->flags[j] == idle)
		j = (j + 1) % n;
	
	spm->turn = j;
	spm->flags[i] = idle;
	
	/* Enter remainder section */
	/* Exit remainder section */
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

void signalHandler(int s) {
//	char buf[4096];
//	strfcat(buf, "palin: Process %d exiting due to %s signal\n", id, s == SIGUSR1 ? "timeout" : "interrupt");
//	printf("%s", buf);
//	exit(1);
//	crash(buf);
	printf("palin: Process %d exiting due to %s signal\n", id, s == SIGUSR1 ? "timeout" : "interrupt");
	exit(EXIT_FAILURE);
}
