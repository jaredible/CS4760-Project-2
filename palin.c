#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool is_palindrome(char *string);

int main(int argc, char **argv) {
//	char *strings[] = {"test", "radar", "Rotor", "aB0bA", "here"};
//	int i;
//	for (i = 0; i < 5; i++) {
//		char *string = strings[i];
//		printf("%s, %s\n", string, is_palindrome(string) ? "true" : "false");
//	}
	
	return EXIT_SUCCESS;
}

bool is_palindrome(char *string) {
	bool result = true;
	
	int index_left = 0, index_right = strlen(string) - 1;
	char char_left, char_right;
	
	while (index_right > index_left) {
		char_left = tolower(string[index_left]);
		char_right = tolower(string[index_right]);
		
		if (char_left != char_right) return false;
		
		index_left++;
		index_right--;
	}
	
	return result;
}
