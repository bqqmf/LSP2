#ifndef MAIN_H_
#define MAIN_H_

// true = 1
#ifndef true
	#define true 1  
#endif
// false = 0
#ifndef false
	#define false 0  
#endif
// stdout = 1
#ifndef STDOUT
	#define STDOUT 1  
#endif
// stderr = 2
#ifndef STDERR
	#define STDERR 2
#endif 
// .txt = 3
#ifndef TEXTFILE
	#define TEXTFILE 3
#endif
// .c = 4
#ifndef CFILE
	#define CFILE 4
#endif
#ifndef OVER
	#define OVER 5
#endif
// warn = -0.1
#ifndef WARNING
	#define WARNING -0.1
#endif
// err = 0
#ifndef ERROR
	#define ERROR 0
#endif

#define FILELEN 128  // len of file
#define BUFLEN 1024  // buf size
#define SNUM 100  // max num of students
#define QNUM 100  // max num of students
#define ARGNUM 5  // max arg num

struct ssu_scoreTable{
	char qname[FILELEN];  // 문제 이름
	double score;		  // 점수
};

void ssu_score(int argc, char *argv[]);
int check_option(int argc, char *argv[]);
void print_usage();

void score_students();
double score_student(int fd, char *id);
void write_first_row(int fd);

char *get_answer(int fd, char *result);
int score_blank(char *id, char *filename);
double score_program(char *id, char *filename);
double compile_program(char *id, char *filename);
int execute_program(char *id, char *filname);
pid_t inBackground(char *name);
double check_error_warning(char *filename);
int compare_resultfile(char *file1, char *file2);

void do_iOption(char (*ids)[FILELEN]);
void do_mOption();
int is_exist(char (*src)[FILELEN], char *target);

int is_thread(char *qname);
void redirection(char *command, int newfd, int oldfd);
int get_file_type(char *filename);
void rmdirs(const char *path);
void to_lower_case(char *c);

void set_scoreTable(char *ansDir);
void read_scoreTable(char *path);
void make_scoreTable(char *ansDir);
void write_scoreTable(char *filename);
void set_idTable(char *stuDir);
int get_create_type();

void sort_idTable(int size);
void sort_scoreTable(int size);
void get_qname_number(char *qname, int *num1, int *num2);

// made by me below
char *to_abs_path(char *path);  // make abs_path
void do_eOption();  // do -e option

#endif
