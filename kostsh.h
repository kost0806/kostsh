#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define INIT_SUCCESS 1;

/* Substitution constance */
#define NO_CONTAIN_HOME 1000
#define SUCCESS_SUB_HOME 1001

int init_shell(char *pwd, char *home, char *user_name);
int command();
int sub_home_dir(char *pwd, char *home);
int get_home(char *home);

extern char **environ;

int init_shell(char *pwd, char *home, char *user_name) {
	pid_t pid = getpid();
	char pwd_tmp[1024];

	user_name = getlogin();

	get_home(home);

	getcwd(pwd_tmp, 1024);

	sub_home_dir(pwd_tmp, home);

	strncpy(pwd, pwd_tmp, 1024);

	return 
}

int command() {

}

int sub_home_dir(char *pwd, char *home) {
	int home_len = strlen(home);
	int i;

	if (!strncmp(pwd, home, home_len)) {
		for (i = 0; i < home_len; ++i) {
			*(pwd + 1 + i) = *(pwd + home_len + i);
		}
		*(pwd + home_len + 1) = '\0';
		*pwd = '~';
		return SUCCESS_SUB_HOME;
	}
	else
		return NO_CONTAIN_HOME;
}

int get_home(char *home) {
	char *home;
	char **envp_tmp = environ;

	while (*envp_tmp != NULL) {
		if (!strncmp(*envp_tmp, "HOME", 4)) {
			if (home == NULL)
				home = (char *)malloc(sizeof(char) * (strlen(*envp_tmp) - 4));
			else 
				home = (char *)realloc(home, sizeof(char) * (strlen(*envp_tmp) - 4));

			strcpy(home, (*envp_tmp) + 5);
		}
		envp_tmp++;
	}
}
