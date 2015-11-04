#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

/* MACROS */
#define pbuf(buf, buf_len, buf_ptr) printf("%s\n", (buf)); memset((buf), 0, (buf_len)); (buf_len)=0; (buf_ptr) = (buf);

/* CONSTANT */
#define MAX_INPUT_SIZE 0x400

/* FLAGS */
#define INIT_SUCCESS 0x0001
#define NO_CONTAIN_HOME 0x1000
#define SUCCESS_SUB_HOME 0x1001

/* TYPEDEF */
typedef enum {NO_BUILTIN, BUILTIN_CD, BUILTIN_PWD, BUILTIN_HIS} func_type;

int init_shell(char *pwd, char *home, char *host_name, char **user_name);
int command(char *input);
int sub_home_dir(char *pwd, char *home);
void display_shell(char *shell_face, char *user_name, char *host_name, char *pwd);
void parse(char *cmd_line);
void get_shell_face(char *shell_face);

extern char **environ;

int init_shell(char *pwd, char *home, char *host_name, char **user_name) {
	pid_t pid = getpid();
	char pwd_tmp[1024];

	*user_name = getlogin();

	home = getenv("HOME");

	getcwd(pwd_tmp, 1024);

	gethostname(host_name, 20);
	
	sub_home_dir(pwd_tmp, home);

	strncpy(pwd, pwd_tmp, 1024);

	return INIT_SUCCESS; 
}

int command(char *input) {
	int pid;
	int status;

	pid = fork();

	if (pid == 0) {
		if (execlp(input, input, NULL) < 0) {
			perror("execlp");
			exit(2);
		}
		exit(0);
	}
	else if (pid > 0) {
		wait(&status);
	}
	else {
		fprintf(stderr, "error!!\n");
	}

	printf("status: %d\n", status >> 8);
	return status >> 8;
}

int sub_home_dir(char *pwd, char *home) {
	int home_len = strlen(home);
	int pwd_len = strlen(pwd);
	int i;

	if (!strncmp(pwd, home, home_len)) {
		for (i = 0; i < pwd_len - home_len; ++i) {
			*(pwd + 1 + i) = *(pwd + home_len + i);
		}
		*(pwd + pwd_len - home_len + 1) = '\0';
		*pwd = '~';
		return SUCCESS_SUB_HOME;
	}
	else
		return NO_CONTAIN_HOME;
}

void display_shell(char *shell_face, char *user_name, char *host_name, char *pwd) {
	fprintf(stdout, shell_face, user_name, host_name, pwd);
}

void parse(char *cmd_line) {
	char buf[256];
	int buf_len = 0;
	char *buf_ptr = buf;
	char *cmd_ptr = cmd_line;

	memset(buf, 0, sizeof(buf));

	while (*cmd_ptr != '\0') {
		switch (*cmd_ptr) {
			case '|' :
				if (buf_len != 0) {
					pbuf(buf, buf_len, buf_ptr);
				}
				printf("|\n");
				cmd_ptr++;
				continue;
			case ';' :
				if (buf_len != 0) {
					pbuf(buf, buf_len, buf_ptr);
				}
				printf(";\n");
				cmd_ptr++;
				continue;
			case '>' :
				if (buf_len != 0) {
					pbuf(buf, buf_len, buf_ptr);
				}
				if (*(cmd_ptr + 1) == '>') {
					cmd_ptr++;
					printf(">>\n");
				}
				else
					printf(">\n");
				cmd_ptr++;
				continue;
			case '<' :
				if (buf_len != 0) {
					pbuf(buf, buf_len, buf_ptr);
				}
				printf("<\n");
				cmd_ptr++;
				continue;
			case '&' :
				if (buf_len != 0) {
					pbuf(buf, buf_len, buf_ptr);
				}
				printf("&\n");
				cmd_ptr++;
				continue;
			case ' ' :
				if (buf_len == 0) {
					cmd_ptr++;
					continue;
				}
				else {
				}	
			default :
				*buf_ptr++ = *cmd_ptr; 
				buf_len++;
				cmd_ptr++;
		}

	}
	if (buf_len != 0) {
		pbuf(buf, buf_len, buf_ptr);
	}
}

void get_shell_face(char *shell_face) {
	uid_t uid = getuid();

	if (uid == 0)
		strcpy(shell_face, "%s@%s:%s# ");
	else
		strcpy(shell_face, "%s@%s:%s$ ");
}
