#ifndef LIB
#define LIB
#include "kostsh.h"
#endif

char *HOME;

int main(int argc, char **argv) {
	char shell_face[256];
	char *user_name;
	char host_name[20];
	char pwd[1024];

	char input_buf[MAX_INPUT_SIZE];
	int input_len;

	get_shell_face(shell_face);
	init_shell(pwd, HOME, host_name, &user_name);

	while (1) {
		display_shell(shell_face, user_name, host_name, pwd);
		fgets(input_buf, MAX_INPUT_SIZE, stdin);
		input_len = strlen(input_buf);
		input_buf[input_len - 1] = '\0';
		input_len--;
		parse(input_buf);
		//if (input_len != 0)
			//command(input_buf);
	}	
}
