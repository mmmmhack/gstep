#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const char* get_default_receiver();

int main(int argc, char* argv[]) {
	const char* progname = argv[0];
	if(argc > 1 && !strcmp(argv[1], "-h")) {
		printf("usage: %s RECEIVER\n", progname);
		exit(0);
	}
	const char* receiver = get_default_receiver();
	if(argc > 1)
		receiver = argv[1];
	printf("hello %s!\n", receiver);
	return 0;
}
