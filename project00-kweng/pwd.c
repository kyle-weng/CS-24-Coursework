#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

int main() {
	char buffer[PATH_MAX];
	if (getcwd(buffer, sizeof(buffer)) == NULL) {
		printf("error in getcwd()\n");
		exit(1);
	}
	else {
		char* curDir = getcwd(buffer, sizeof(buffer));
		printf("%s\n", curDir);
	}
	return 0;
}