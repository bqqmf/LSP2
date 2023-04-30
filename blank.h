#ifndef BLANK_H_
#define BLANK_H_

#ifndef true
	#define true 1
#endif
#ifndef false
	#define false 0
#endif
#ifndef BUFLEN
	#define BUFLEN 1024
#endif

#define OPERATOR_CNT 24
#define DATATYPE_SIZE 35
#define MINLEN 64
#define TOKEN_CNT 50

/* only child_head can access parent node */
/* LCRS tree by double linked list        */
typedef struct node{
	int parentheses;		  /* ( ) flag           */
	char *name;				  /* node's name		*/
	struct node *parent;      /* point parent node  */
	struct node *child_head;  /* point child's head */
	struct node *prev;		  /* point prev node    */
	struct node *next;		  /* point next node	*/
}node;

typedef struct operator_precedence{
	char *operator;  /* operator		      */
	int precedence;  /* operator's precedende */
}operator_precedence;

void compare_tree(node *root1,  node *root2, int *result);  // root1 트리와 root2 트리 비교
node *make_tree(node *root, char (*tokens)[MINLEN], int *idx, int parentheses);  // 답안 토큰들로 tree 생성
node *change_sibling(node *parent);  // parent의 첫째, 둘째 노드를 바꾼다
node *create_node(char *name, int parentheses);  // 노드 생성
int get_precedence(char *op);  // op의 우선순의 리턴
int is_operator(char *op);  // op가 연산자인지
void print(node *cur);  // cur 내용 출력
node *get_operator(node *cur);  // cur의 부모인 연산자 노드 리턴
node *get_root(node *cur);  // cur의 root 노드 리턴
node *get_high_precedence_node(node *cur, node *new);  // cur, new 중 높은 우선순위 가진 노드 리턴
node *get_most_high_precedence_node(node *cur, node *new);  // cur, new 중 가장 높은 우선순위 가진 노드 리턴
node *insert_node(node *old, node *new);  // old의 prev에 new 노드 삽입
node *get_last_child(node *cur);  // 마지막 노드 리턴
void free_node(node *cur);  // 노드 해제
int get_sibling_cnt(node *cur);  // 형제 노드 개수 리턴

int make_tokens(char *str, char tokens[TOKEN_CNT][MINLEN]);  // tokens 채우기
int is_typeStatement(char *str);  // 어떤 typeStatement인지 리턴
int find_typeSpecifier(char tokens[TOKEN_CNT][MINLEN]);  // tokens에서 datatype의 인덱스 리턴
int find_typeSpecifier2(char tokens[TOKEN_CNT][MINLEN]);  // tokens에서 struct 인덱스 리턴
int is_character(char c);  // c가 문자인지 리턴
int all_star(char *str);  // str이 모두 *인지 리턴
int all_character(char *str);  // str이 모두 ch인지 리턴
int reset_tokens(int start, char tokens[TOKEN_CNT][MINLEN]);  // start부터 tokens에 토큰 쪼개기
void clear_tokens(char tokens[TOKEN_CNT][MINLEN]);  // tokens 초기화
int get_token_cnt(char tokens[TOKEN_CNT][MINLEN]);  // token 개수 리턴
char *rtrim(char *_str);  // 오른쪽 공백 제거
char *ltrim(char *_str);  // 왼쪽 공백 제거
void remove_space(char *str);  // 공백 제거
int check_brackets(char *str);  // 괄호 검사
char* remove_extraspace(char *str);  // 여분 공백 제거

#endif
