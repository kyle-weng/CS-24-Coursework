#include <stdio.h>

int main(int argc, char* argv[]) {
	if (argc == 1) {
		printf("ERROR\n");
		return 0;
	}
	printf("%s\n",argv[1]);
	return 0;
}