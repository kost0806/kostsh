#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>

/* MACROS */
#define pbuf(buf, buf_len, buf_ptr) printf("%s\n", (buf)); memset((buf), 0, (buf_len)); (buf_len)=0; (buf_ptr) = (buf);

/* CONSTANT */
#define MAX_INPUT_SIZE 0x400
#define MAX_THREAD_NUM 20

/* FLAGS */
#define INIT_SUCCESS 0x0001
#define NO_CONTAIN_HOME 0x1000
#define SUCCESS_SUB_HOME 0x1001
#define SUB_TO_HOME 0x1100
#define SUB_TO_CURRENT 0x1101
#define SUB_TO_PRE 0x1102
#define SUB_NO_SUB 0x1103
#define CD_SUCCESS 0x2000
#define CD_FAILED 0x2001
#define PWD_SUCCESS 0x2002
#define PWD_FAILED 0x2003
#define HIS_WRITE_FAIL 0x3000
#define HIS_SUCCESS 0x3001
#define NOT_IMPLEMENTED 0xFFFF

/* TYPEDEF */
typedef enum {NO_BUILTIN, BUILTIN_CD, BUILTIN_PWD, BUILTIN_HIS, BUILTIN_EXIT} func_type;
typedef enum {NON_PIPE, BEFORE_PIPE, AFTER_PIPE, BETWEEN_PIPE} pipe_flag;
typedef enum {NON_REDI, REDI, REDI_TRGT, REDIAPPEND, REDIAPPEND_TRGT, REDIIN, REDIEX, REDIEX_TRGT} redi_flag;
typedef enum {NON_BACKGROUND, BACKGROUND} back_flag;

/* FUNCTIONS */
int init_shell(char *pwd, char *home, char *host_name, char **user_name);
int command(char *input);
int sub_home_dir(char *pwd, char *home);
void display_shell();
void parse(char *cmd_line);
int sub_directory(char *pwd, char *buf);
func_type get_type(char *file);
void *check_child(int *p); 

/* BUILTIN FUNCTIONS */
int builtin_cd(char *arg);
int builtin_pwd();
int builtin_exit();
int builtin_history();

/* EXTERN */
extern char **environ;

/* GLOBAL VARIABLES */
int pfd[2][2];
pipe_flag pflag= NON_PIPE;
redi_flag rflag = NON_REDI;
back_flag bflag = NON_BACKGROUND;
int pturn = 0; // mod 2
int history_fd;
int back_pnum = 0;
char redi_pname[256];
pthread_t thread[MAX_THREAD_NUM];
int thread_num = 0;

int init_shell(char *pwd, char *home, char *host_name, char **user_name) {
	pid_t pid = getpid();
	char pwd_tmp[1024];

	char his_tmp[1024];

	*user_name = getlogin();

	home = getenv("HOME");

	getcwd(pwd_tmp, 1024);

	gethostname(host_name, 20);

	sub_home_dir(pwd_tmp, home);

	strncpy(pwd, pwd_tmp, 1024);

	strcpy(his_tmp, getenv("HOME"));
	strcat(his_tmp, "/.kostsh_history");

	if ((history_fd = open(his_tmp, O_RDWR|O_CREAT, 0644)) < 0) {
		fprintf(stderr, "Shell initializing failed\n");
	}

	return INIT_SUCCESS; 
}

int command(char *input) {
	int pid;
	int status;

	int i;

	char *arg[20];
	char *input_ptr;
	int input_len = strlen(input);
	int tok_num = 0;

	int t_tmp[2];
	int k;

	memset(arg, 0, sizeof(arg));

	/* trim */
	input_ptr = input + input_len - 1;
	while (*input_ptr == ' ') {
		*input_ptr-- = '\0';
		input_len--;
	}

	/* separate token */
	input_ptr = input;
	i = 0;
	while (1) {
		if (*input_ptr == '\0') {
			arg[tok_num] = (char *)malloc(sizeof(char) * (i + 1));
			strncpy(arg[tok_num], input_ptr - i, i);
			*(arg[tok_num] + i) = '\0';
			tok_num++;
			break;
		}
		if (*input_ptr == ' ') {
			arg[tok_num] = (char *)malloc(sizeof(char) * (i + 1));
			strncpy(arg[tok_num], input_ptr - i, i);
			*(arg[tok_num] + i) = '\0';
			tok_num++;
			input_ptr++;
			i = 0;
		}
		i++;
		input_ptr++;
	}
	arg[tok_num] = NULL;

	switch(get_type(arg[0])) {
		case BUILTIN_CD :
			return builtin_cd(arg[1]);
		case BUILTIN_PWD :	
			return builtin_pwd();
		case BUILTIN_HIS :
			return builtin_history();
		case BUILTIN_EXIT :
			builtin_exit();
			return 0;
		default :
			break;
	}

	pid = fork();

	if (pid == 0) {
		/* IF PIPE FLAG IS ON */
		switch (pflag) {
			case BEFORE_PIPE :
				if (close(1) == -1)
					perror("close");
				if (dup(pfd[pturn][1]) != 1)
					perror("dup1");
				close(pfd[pturn][0]);
				close(pfd[pturn][1]);
				break;
			case AFTER_PIPE :
				if (close(0) == -1)
					perror("close");
				if (dup(pfd[pturn][0]) != 0)
					perror("dup2");
				close(pfd[pturn][0]);
				close(pfd[pturn][1]);
				break;
			case BETWEEN_PIPE :
				close(0);
				dup(pfd[(pturn + 1) % 2][0]);
				close(1);
				dup(pfd[pturn][1]);
				close(pfd[pturn][0]);
				close(pfd[pturn][1]);
				close(pfd[(pturn + 1) % 2][0]);
				close(pfd[(pturn + 1) % 2][1]);
				break;
		}

		/* IF REDIRECTION FLAG IS ON */
		switch (rflag) {
			case REDI :
			case REDIAPPEND :
				close(1);
				close(2);
				dup(pfd[pturn][1]);
				dup(pfd[pturn][1]);
				close(pfd[pturn][0]);
				close(pfd[pturn][1]);
				break;
			case REDIEX :
				close(1);
				close(2);
				dup(pfd[pturn][1]);
				dup(pfd[pturn][1]);
				close(pfd[pturn][0]);
				close(pfd[pturn][1]);
				break;
			case REDIIN :
				close(0);
				dup(pfd[pturn][0]);
				close(pfd[pturn][0]);
				close(pfd[pturn][1]);
				break;
		}

		/* EXECUTE PROGRAM */
		if (execvp(arg[0], arg) < 0) {
			perror("execvp");
			exit(2);
		}
	}
	else if (pid > 0) {
		if (pflag == BEFORE_PIPE)
			pflag = AFTER_PIPE;
		else if (pflag == AFTER_PIPE) {
			pflag = NON_PIPE;
			close(pfd[pturn][0]);
			close(pfd[pturn][1]);
		}
		else if (pflag == BETWEEN_PIPE) {
			pflag = AFTER_PIPE;
			close(pfd[(pturn + 1) % 2][0]);
			close(pfd[(pturn + 1) % 2][1]);
		}
		switch (rflag) {
			case REDI :
				rflag = REDI_TRGT;
				break;
			case REDIAPPEND :
				rflag = REDIAPPEND_TRGT;
				break;
			case REDIEX :
				rflag = REDIEX_TRGT;
				break;
			case REDIIN :
				close(pfd[pturn][0]);
				close(pfd[pturn][1]);
				break;
		}
		if (bflag == BACKGROUND) {
			t_tmp[0] = back_pnum;
			t_tmp[1] = pid;
			fprintf(stdout, "[%d] %d\n", back_pnum, pid);
			if (pthread_create(&thread[thread_num], NULL, check_child, t_tmp) < 0) {
				fprintf(stderr, "error occur!\n");
				return;
			}
			back_pnum++;
			pthread_detach(thread[thread_num]);
			thread_num++;
			bflag = NON_BACKGROUND;
		}
		else
			wait(&status);
	}
	else {
		fprintf(stderr, "error!!\n");
	}

	for (k = 0; k < tok_num; ++k) {
		free(arg[k]);
	}

	//printf("status: %d\n", status >> 8);
	return status;
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

void display_shell() {
	char *shell_face_user = "%s@%s:%s$ ";
	char *shell_face_root = "%s@%s:%s# ";
	char *user_name = getlogin();
	char host_name[30];
	char *pwd_env = getenv("PWD");
	char pwd[strlen(pwd_env) + 1];
	uid_t uid = getuid();

	gethostname(host_name, 30);

	strcpy(pwd, pwd_env);

	sub_home_dir(pwd, getenv("HOME"));

	if (uid == 0)
		fprintf(stdout, shell_face_root, user_name, host_name, pwd);
	else
		fprintf(stdout, shell_face_user, user_name, host_name, pwd);
}

void parse(char *cmd_line) {
	char buf[256];
	int buf_len = 0;
	char *buf_ptr = buf;
	char *cmd_ptr = cmd_line;

	int redi_fd;
	char redi_buf[512];
	int redi_line;

	memset(buf, 0, sizeof(buf));

	write(history_fd, cmd_line, strlen(cmd_line));
	write(history_fd, "\n", 1);

	while (*cmd_ptr != '\0') {
		switch (*cmd_ptr) {
			case '|' :
				if (buf_len != 0) {
					if (pflag == NON_PIPE) {
						if (pipe(pfd[pturn]) == -1) {
							perror("pipe");
							exit(0xFF);
						}
						pflag = BEFORE_PIPE;
					}
					if (pflag == AFTER_PIPE) {
						pturn = (pturn + 1) % 2;
						pipe(pfd[pturn]);
						pflag = BETWEEN_PIPE;
					}
					command(buf);
					memset(buf, 0, buf_len); 
					buf_len = 0; 
					buf_ptr = buf;
				}
				cmd_ptr++;
				continue;
			case ';' :
				if (buf_len != 0) {
					command(buf);
					memset(buf, 0, buf_len); 
					buf_len = 0; 
					buf_ptr = buf;
				}
				cmd_ptr++;
				continue;
			case '>' :
				if (*(cmd_ptr + 1) == '>') {
					rflag = REDIAPPEND;
					cmd_ptr++;
				}
				else if (*(cmd_ptr + 1) == '!') {
					rflag = REDIEX;
					cmd_ptr++;
				}
				else {
					rflag = REDI;
				}
				pipe(pfd[pturn]);
				if (buf_len != 0) {
					command(buf);
					memset(buf, 0, buf_len); 
					buf_len = 0; 
					buf_ptr = buf;
				}
				cmd_ptr++;
				continue;
			case '<' :
				if (buf_len != 0) {
					strcpy(redi_pname, buf);
					rflag = REDIIN;
					memset(buf, 0, buf_len); 
					buf_len = 0; 
					buf_ptr = buf;
				}
				cmd_ptr++;
				continue;
			case '&' :
				if (buf_len != 0) {
					bflag = BACKGROUND;
					command(buf);
					memset(buf, 0, buf_len); 
					buf_len = 0; 
					buf_ptr = buf;
				}
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

	switch (rflag) {
		case REDI_TRGT : 
			rflag = NON_REDI;
			sub_directory(buf, redi_buf);
			if (!access(redi_buf, F_OK)) {
				fprintf(stderr, "File exists: %s\n", buf);
				return;
			}
			if ((redi_fd = open(redi_buf, O_WRONLY|O_CREAT, 0644)) < 0) {
				//perror("open");
				fprintf(stderr, "failed redirect\n");
				return;
			}
			while ((redi_line = read(pfd[pturn][0], redi_buf, 512)) > 0) {
				if (write(redi_fd, redi_buf, redi_line) < 0) {
					perror("write");
					fprintf(stderr, "error occur\n");
				}
				if (redi_line < 512)
					break;
			}
			close(pfd[pturn][0]);
			close(pfd[pturn][1]);
			close(redi_fd);
			memset(buf, 0, buf_len);
			buf_len = 0;
			buf_ptr = buf;
			break;
		case REDIAPPEND_TRGT :
			rflag = NON_REDI;
			sub_directory(buf, redi_buf);
			if ((redi_fd = open(redi_buf, O_WRONLY|O_CREAT|O_APPEND, 0644)) < 0) {
				//perror("open");
				fprintf(stderr, "failed redirect\n");
				return;
			}
			while ((redi_line = read(pfd[pturn][0], redi_buf, 512)) > 0) {
				if (write(redi_fd, redi_buf, redi_line) < 0) {
					perror("write");
					fprintf(stderr, "error occur\n");
				}
				if (redi_line < 512)
					break;
			}
			close(pfd[pturn][0]);
			close(pfd[pturn][1]);
			close(redi_fd);
			memset(buf, 0, buf_len);
			buf_len = 0;
			buf_ptr = buf;
			break;
		case REDIEX_TRGT :
			rflag = NON_REDI;
			sub_directory(buf, redi_buf);
			if (!access(redi_buf, F_OK)) {
				remove(redi_buf);
			}
			if ((redi_fd = open(redi_buf, O_WRONLY|O_CREAT, 0644)) < 0) {
				//perror("open");
				fprintf(stderr, "failed redirect\n");
				return;
			}
			while ((redi_line = read(pfd[pturn][0], redi_buf, 512)) > 0) {
				if (write(redi_fd, redi_buf, redi_line) < 0) {
					perror("write");
					fprintf(stderr, "error occur\n");
				}
				if (redi_line < 512)
					break;
			}
			close(pfd[pturn][0]);
			close(pfd[pturn][1]);
			close(redi_fd);
			memset(buf, 0, buf_len);
			buf_len = 0;
			buf_ptr = buf;
			break;
		case REDIIN :
			pipe(pfd[pturn]);
			sub_directory(buf, redi_buf);
			if ((redi_fd = open(redi_buf, O_RDONLY|O_NONBLOCK)) < 0) {
				fprintf(stderr, "Redirection: %s: No such file or directory\n", redi_buf);
				return;
			}
			while ((redi_line = read(redi_fd, redi_buf, 512)) > 0) {
				if (write(pfd[pturn][1], redi_buf, redi_line) < 0) {
					perror("write");
				}
			}
			close(redi_fd);
			command(redi_pname);
			rflag = NON_REDI;
			buf_len = 0;
	}


	if (buf_len != 0) {
		command(buf);
		memset(buf, 0, buf_len); 
		buf_len = 0; 
		buf_ptr = buf;
	}
}

func_type get_type(char *file) {
	if (!strcmp(file, "cd"))
		return BUILTIN_CD;
	else if (!strcmp(file, "pwd"))
		return BUILTIN_PWD;
	else if (!strcmp(file, "history"))
		return BUILTIN_HIS;
	else if (!strcmp(file, "exit"))
		return BUILTIN_EXIT;
	else
		return NO_BUILTIN;
}

int sub_directory(char *pwd, char *buf) {
	char *pwd_env = getenv("PWD");
	char *pwd_ptr;

	if (pwd == NULL) {
		strcpy(buf, getenv("HOME"));
		return SUB_TO_HOME;
	}
	else if (!strncmp(pwd, "./", 2)) {
		strcpy(buf, pwd_env);
		strcat(buf, pwd + 1);
		return SUB_TO_CURRENT;
	}
	else if (!strncmp(pwd, "..", 2)) {
		pwd_ptr = pwd_env + strlen(pwd_env);
		while (*pwd_ptr-- != '/');
		if (pwd_ptr < pwd_env)
			pwd_ptr++;
		strncpy(buf, pwd_env, pwd_ptr - pwd_env + 1);
		strcat(buf, pwd + 2);
		return SUB_TO_PRE;
	}
	else if (!strncmp(pwd, "~", 1)) {
		strcpy(buf, getenv("HOME"));
		strcat(buf, pwd + 1);
		return SUB_TO_HOME;
	}
	else if (*pwd != '/') {
		strcpy(buf, pwd_env);
		strcat(buf, "/");
		strcat(buf,pwd);
		return SUB_TO_CURRENT;
	}
	else {
		strcpy(buf, pwd);
		return SUB_NO_SUB;
	}

}

/* builtin functions */
int builtin_cd(char *arg) {
	char want_to_go[1024];
	char pwd[1024];
	char *pwd_ptr;
	char *pwd_env = getenv("PWD");

	memset(want_to_go, 0, 1024);

	sub_directory(arg, want_to_go);

	if (chdir(want_to_go) < 0) {
		if (strcmp("..", arg))
			fprintf(stderr, "-kostsh: cd: %s: No such file of directory\n", arg);
		return CD_FAILED;
	}
	else {
		strcpy(pwd, want_to_go);
		sprintf(pwd, "PWD=%s", want_to_go);
		putenv(pwd);
		return CD_SUCCESS;
	}
}

int builtin_pwd() {
	char *pwd_env = getenv("PWD");

	if (pwd_env == NULL)
		return PWD_FAILED;

	fprintf(stdout, "%s\n", getenv("PWD"));
	return PWD_SUCCESS;
}

int builtin_exit() {
	exit(0);
}

int builtin_history() {
	char buf[512];
	char n;

	lseek(history_fd, 0, SEEK_SET);


	while((n = read(history_fd, buf, 512)) > 0) {
		if (write(1, buf, n) < 0) {
			lseek(history_fd, 0, SEEK_END);
			return HIS_WRITE_FAIL;
		}
	}

	lseek(history_fd, 0, SEEK_END);
	return HIS_SUCCESS;
}

void *check_child(int *p) {
	int pnum = *(p);
	int pid= *(p + 1);
	waitpid(pid, NULL, 0);
	printf("[%d]	done		%d\n", pnum, pid);
	back_pnum--;
	thread_num--;
}
