#include <stdio.h>


int main(int argc, char *argv[]) { 

	printf("char prog[] = {\n");
	int moving_c;
	size_t index = 0;
	while ((moving_c = fgetc(stdin)) != EOF) {
		printf("\t%d,%c", moving_c, (index+1)%8==0 ? '\n' : ' ');
		index++;
	}
	printf("0 };\n");
	return 0;
}