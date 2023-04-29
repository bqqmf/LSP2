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
#define IDLEN     // STUDENT_ID len

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

void do_pOption(char (*ids)[IDLEN], char *target_id);
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
typedef struct Q_node Q_node;
typedef struct ID_node ID_node;
typedef struct sorted_node sorted_node;
struct Q_node {
	Q_node *next;     // 다음 문제 노드
	double result;    // 채점 결과
	double score;     // 배점
	char qname[128];  // 문제 이름
};
struct ID_node {
	ID_node *next;  // 다음 학생 노드
	Q_node *child;  // 학생의 첫 문제 노드
	int sorted;     // 정렬 여부
	double score;   // 총점 
	char id[10];    // 학번
};

char *to_abs_path(char *path);  // make abs_path
void do_eOption();  // do -e option
void create_ANS_STD_Dir();  // create ./ANS, ./STD dir
int print_with_score(char *id);  // return id is in c_opt_students.
int getStudentIDs(char (*IDlist)[IDLEN]);  // save IDs in STD_DIR to IDlist and return num of students
void sort_2Darray(char (*arr)[], int size);  // sort 2D array. to sort iIDs

// linked list func in pOption
void print_pOption(char *id);  // print id's wrong qname and baejum
ID_node *create_id_node(char *id);  // create id node
void add_id_node(ID_node *new);  // add node
Q_node *create_q_node(char *qname, double result, double score);
void add_q_node(ID_node *parent, Q_node *new);
ID_node *find_node_by_id(char *id);
void free_id_node();
void free_q_node(Q_node *del);
int in_iIDs(char *id);
int in_c_students(char *id);
void do_sOption();
void sort_linked_list();

#endif
