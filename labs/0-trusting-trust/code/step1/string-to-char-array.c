// convert the contents of stdin to their ASCII values (e.g., 
// '\n' = 10) and spit out the <prog> array used in Figure 1 in
// Thompson's paper.
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
