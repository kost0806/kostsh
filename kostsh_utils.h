#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <wait.h>

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
void *check_child(void *p); 

/* BUILTIN FUNCTIONS */
int builtin_cd(char *arg);
int builtin_pwd();
int builtin_exit();
int builtin_history();
