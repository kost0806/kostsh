#ifndef LIB
#define LIB
#include "kostsh.h"
#endif

char *HOME;

int main(int argc, char **argv) {
	char shell_face[1024] = "%s@%s$ ";
	char *user_name;
	char pwd[1024];

	init_shell(pwd, HOME, user_name);
	
	printf("\n");
	
}
