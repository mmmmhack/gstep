#include <stdio.h>

int main(int argc, char* argv[]) {
	const char* receiver = "world";
	if(argc > 1)
		receiver = argv[1];
	printf("hello %s!\n", receiver);
	return 0;
}
