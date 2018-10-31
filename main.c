#ifndef LIB
#define LIB
#include "kostsh_utils.h"
#endif

char *HOME;
char pwd[1024];

int main(int argc, char **argv) {
	char shell_face[256];
	char *user_name;
	char host_name[20];

	char input_buf[MAX_INPUT_SIZE];
	int input_len;

	init_shell(pwd, HOME, host_name, &user_name);

	while (1) {
		display_shell();
		fgets(input_buf, MAX_INPUT_SIZE, stdin);
		input_len = strlen(input_buf);
		input_buf[input_len - 1] = '\0';
		input_len--;
		if (input_len == 0)
			continue;
		parse(input_buf);
	}	
}
