#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "blank.h"

// save data types
char datatype[DATATYPE_SIZE][MINLEN] = {"int", "char", "double", "float", "long"
			, "short", "ushort", "FILE", "DIR","pid"
			,"key_t", "ssize_t", "mode_t", "ino_t", "dev_t"
			, "nlink_t", "uid_t", "gid_t", "time_t", "blksize_t"
			, "blkcnt_t", "pid_t", "pthread_mutex_t", "pthread_cond_t", "pthread_t"
			, "void", "size_t", "unsigned", "sigset_t", "sigjmp_buf"
			, "rlim_t", "jmp_buf", "sig_atomic_t", "clock_t", "struct"};


// operator rank
operator_precedence operators[OPERATOR_CNT] = {
	{"(", 0}, {")", 0}
	,{"->", 1}	
	,{"*", 4}	,{"/", 3}	,{"%", 2}	
	,{"+", 6}	,{"-", 5}	
	,{"<", 7}	,{"<=", 7}	,{">", 7}	,{">=", 7}
	,{"==", 8}	,{"!=", 8}
	,{"&", 9}
	,{"^", 10}
	,{"|", 11}
	,{"&&", 12}
	,{"||", 13}
	,{"=", 14}	,{"+=", 14}	,{"-=", 14}	,{"&=", 14}	,{"|=", 14}
};

/* score_blank() 에서 호출됨 */
/* root1 : 학생 답안 트리, root2 : 모범 답안 트리, result : 정답 여부 저장 */
void compare_tree(node *root1,  node *root2, int *result)
{
	node *tmp;  // 임시 노드
	int cnt1, cnt2;  // root1과 root2의 개수

	if(root1 == NULL || root2 == NULL){  // 트리가 비었으면 오답 처리
		*result = false;
		return;
	}

	/* when root1's name is < or > or <= or >= */
	// 연산자 순서를 동일하게 맞추나보다
	if(!strcmp(root1->name, "<") || !strcmp(root1->name, ">") || !strcmp(root1->name, "<=") || !strcmp(root1->name, ">=")){
		// when root1's name and root2's name are different
		if(strcmp(root1->name, root2->name) != 0){
			// if first char is < then change to >
			if(!strncmp(root2->name, "<", 1))
				strncpy(root2->name, ">", 1);

			// if first char is > then change to >
			else if(!strncmp(root2->name, ">", 1))
				strncpy(root2->name, "<", 1);

			// if first char is <= then change to >=
			else if(!strncmp(root2->name, "<=", 2))
				strncpy(root2->name, ">=", 2);

			// if first char is >= then change to <=
			else if(!strncmp(root2->name, ">=", 2))
				strncpy(root2->name, "<=", 2);

			root2 = change_sibling(root2);  // 형제 노드 변경
		}
	}

	if(strcmp(root1->name, root2->name) != 0){  // 첫 노드가 다르면 오답
		*result = false;
		return;
	}

	// 학생 트리와 답안 트리의 자식 여부가 일치하지 않으면 오답
	if((root1->child_head != NULL && root2->child_head == NULL)
		|| (root1->child_head == NULL && root2->child_head != NULL)){
		*result = false;
		return;
	}
	// 두 트리의 자식 여부 일치
	else if(root1->child_head != NULL){  // 자식이 있다면
		// 자식의 수가 다르면 오답
		if(get_sibling_cnt(root1->child_head) != get_sibling_cnt(root2->child_head)){
			*result = false;
			return;
		}
		// 현재 노드가 ==나 != 라면
		if(!strcmp(root1->name, "==") || !strcmp(root1->name, "!="))
		{
			// 자식에 대해 트리 비교
			compare_tree(root1->child_head, root2->child_head, result);

			// a == b와 b == a가 같으니 아래 수행
			if(*result == false)  // 자식트리가 다르면
			{
				*result = true;  // 현재 트리는 같다고 놓고
				root2 = change_sibling(root2);  // root2의 첫째 둘째 바꾸기
				compare_tree(root1->child_head, root2->child_head, result);  // 자식 다시 비교
			}
		}
		// 현재 노드가 아래 6개중 하나라면
		else if(!strcmp(root1->name, "+") || !strcmp(root1->name, "*")
				|| !strcmp(root1->name, "|") || !strcmp(root1->name, "&")
				|| !strcmp(root1->name, "||") || !strcmp(root1->name, "&&"))
		{
			// 피 연산자 수가 다르면 오답
			if(get_sibling_cnt(root1->child_head) != get_sibling_cnt(root2->child_head)){
				*result = false;
				return;
			}
			// 피 연산자의 수가 같은 경우

			tmp = root2->child_head;  // tmp는 피연산자의 첫째를 가리킴

			while(tmp->prev != NULL)  // 가장 왼쪽 노드로 이동. 첫쨰인데 왜 이동하지?
				tmp = tmp->prev;

			while(tmp != NULL)  
			{
				compare_tree(root1->child_head, tmp, result);  // root1과 tmp를 비교
			
				if(*result == true)  // 정답이면 break;
					break;
				else{
					if(tmp->next != NULL)  // 다음 노드가 없으면 정답
						*result = true;
					tmp = tmp->next;  // 다음 노드 가리키기
				}
			}
		}
		else{  // 자식들에 대해 재귀적으로 비교
			compare_tree(root1->child_head, root2->child_head, result);
		}
	}	

	if(root1->next != NULL){  // 형제가 있다면

		if(get_sibling_cnt(root1) != get_sibling_cnt(root2)){  // 형재 수가 다르면 오답
			*result = false;
			return;
		}

		if(*result == true)
		{
			tmp = get_operator(root1);  // root1 형제들의 부모 노드 반환
			
			// root1의 부모 노드가 아래 연산자 중 하나라면 
			if(!strcmp(tmp->name, "+") || !strcmp(tmp->name, "*")
					|| !strcmp(tmp->name, "|") || !strcmp(tmp->name, "&")
					|| !strcmp(tmp->name, "||") || !strcmp(tmp->name, "&&"))
			{	
				tmp = root2;  // 2번째 root 가리키기
	
				while(tmp->prev != NULL)  // 맨 앞 노드로 이동
					tmp = tmp->prev;

				while(tmp != NULL)
				{
					compare_tree(root1->next, tmp, result);  // 다음 노드와 비교

					if(*result == true)  // 정답이면 true
						break;
					else{
						if(tmp->next != NULL)  // 다음 노드가 없으면 정답
							*result = true;
						tmp = tmp->next;  // 다음 노드로 이동
					}
				}
			}

			else  // 형제 비교
				compare_tree(root1->next, root2->next, result);
		}
	}
}

/* score_blank()에서 호출됨 */
/* 학생의 답안을 쪼개서 tokens에 저장 */
/* str : 학생 제출 답안, tokens : 토큰 저장 변수 */
int make_tokens(char *str, char tokens[TOKEN_CNT][MINLEN])
{
	char *start, *end;  // primary pointer to str
	char tmp[BUFLEN];  // save op temporary
	char str2[BUFLEN];
	char *op = "(),;><=!|&^/+-*\"";  // 연산자들
	int row = 0;  // 현재 저장한 토큰 수
	int i;
 	int isPointer;  // current token is pointer type
	int lcount, rcount;  // (, ) 개수
	int p_str;
	
	clear_tokens(tokens);  // tokens를 0으로 초기화

	start = str;  // 학생 제출 답안 문자열을 가리킨다
	
	if(is_typeStatement(str) == 0)  // str이 Statement가 아니라 틀린 답인지 리턴
		return false;  // 오답
	
	while(1)
	{
		// start는 앞으로 더 쪼갤 답안의 맨 앞을 가리킴
		// end는 start로부터 가장 먼저 나오는 연산자의 위치를 가리킴
		if((end = strpbrk(start, op)) == NULL)  // 학생 답안에 연산자가 없다면 break
			break;

		if(start == end){  // start의 첫 글자가 op라면

			if(!strncmp(start, "--", 2) || !strncmp(start, "++", 2)){  // 연산자가 -- ++ 이라면
				if(!strncmp(start, "++++", 4)||!strncmp(start,"----",4))  // s_ans starts with ++++ or ----
					return false;  // wrong answer

				// ex) ++a
				if(is_character(*ltrim(start + 2))){  // if char behind ++ [0-9a-zA-Z]
					if(row > 0 && is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1]))
						return false; //ex) ++a++? a ++ a

					end = strpbrk(start + 2, op);  // find next op after ++
					if(end == NULL)  // there's no op excepts for front ++
						end = &str[strlen(str)];  // end points end of s_ans
					// save tokens from s_ans
					while(start < end) {
						if(*(start - 1) == ' ' && is_character(tokens[row][strlen(tokens[row]) - 1]))  // prev char is space && current token's last char is [0-9a-Z]
							return false;
						else if(*start != ' ')  // save current char
							strncat(tokens[row], start, 1);  // concat current char to row'th token
						start++;  // points next char
					}
				}
				// ex) a++
				else if(row>0 && is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])){  // not first token and prev token is [0-9a-zA-Z]
					if(strstr(tokens[row - 1], "++") != NULL || strstr(tokens[row - 1], "--") != NULL)	// if prev token has ++ or --
						return false;  // ++ a ++ is error

					memset(tmp, 0, sizeof(tmp));  // clear tmp
					strncpy(tmp, start, 2);  // save ++ or -- in tmp
					strcat(tokens[row - 1], tmp);  // attach tmp to prev token
					start += 2;  // inc start index
					row--;  // point prev token
				}
				else{
					memset(tmp, 0, sizeof(tmp));  // clear tmp
					strncpy(tmp, start, 2);  // save ++ or -- in tmp
					strcat(tokens[row], tmp);  // attach tmp to prev token
					start += 2;  // inc start index
				}
			}

			else if(!strncmp(start, "==", 2) || !strncmp(start, "!=", 2) || !strncmp(start, "<=", 2)
				|| !strncmp(start, ">=", 2) || !strncmp(start, "||", 2) || !strncmp(start, "&&", 2) 
				|| !strncmp(start, "&=", 2) || !strncmp(start, "^=", 2) || !strncmp(start, "!=", 2) 
				|| !strncmp(start, "|=", 2) || !strncmp(start, "+=", 2)	|| !strncmp(start, "-=", 2) 
				|| !strncmp(start, "*=", 2) || !strncmp(start, "/=", 2)){  // 연산자가 복합 대입 연산자 중 하나라면

				strncpy(tokens[row], start, 2);  // 토큰에 연산자 추가
				start += 2;  // 추가한 연산자 뒤를 가리킴
			}
			// a->b에서 ->를 만나면 이전 토큰 a에 ->b를 붙여준다
			else if(!strncmp(start, "->", 2))  // 연산자가 -> 라면
			{
				end = strpbrk(start + 2, op);  // end는 -> 뒤의 연산자를 가리킴

				if(end == NULL)  // -> 뒤에 연산자가 나오지 않는다면
					end = &str[strlen(str)];  // end는 str의 맨 뒤를 가리킨다

				while(start < end){  // start 부터 다음 연산자나 str의 끝까지
					if(*start != ' ')  // space가 아니라면
						strncat(tokens[row - 1], start, 1); // 이전 토큰에 ->b 추가
					start++;  // 다음 문자 가리키기
				}
				row--;  // 이전 토큰에 추가 했으니 1 감소 뒤에서 1 추가함
			}
			else if(*end == '&')  // 연산자가 &라면
			{
				// ex) &a (address)
				// 첫 번째 토큰 추가할 차례거나 이전 토큰에 연산자가 있다면
				if(row == 0 || (strpbrk(tokens[row - 1], op) != NULL)){
					end = strpbrk(start + 1, op);  // end는 & 이후 연산자를 가리킴
					if(end == NULL)  // & 이후 연산자가 없다면
						end = &str[strlen(str)];  // str의 끝을 가리킴
					
					strncat(tokens[row], start, 1);  // 토큰에 & 추가
					start++;  // & 읽기 완료

					while(start < end){  // & 뒤부터 end까지
						if(*(start - 1) == ' ' && tokens[row][strlen(tokens[row]) - 1] != '&')
							return false;  // 이 조건은 나중에 보자 마지막 글자가 &이 아닌데 이전 글자가 공백? &a b 이런 상황인가
						else if(*start != ' ')  // space가 아니면
							strncat(tokens[row], start, 1);  // 토큰에 문자 추가
						start++;  // 다음 문자 가리키기
					}
				}
				// ex) a & b (bit)
				else{
					strncpy(tokens[row], start, 1);  // 토큰에 문자 추가
					start += 1;  // 다음 문자 가리키기
				}
				
			}
		  	else if(*end == '*')  // 연산자가 *이라면
			{
				isPointer=0;  // save either token is pointer

				if(row > 0)  // not first token
				{
					//ex) char** (pointer)
					for(i = 0; i < DATATYPE_SIZE; i++) { 
						if(strstr(tokens[row - 1], datatype[i]) != NULL){  // ex) int*, char*, ...
							strcat(tokens[row - 1], "*");  // attach
							start += 1;	 // inc start
							isPointer = 1;  // current token is pointer
							break;
						}
					}
					if(isPointer == 1)  // if current token is pointer type, check next token
						continue;
					if(*(start+1) !=0)  // if ' ' next to *
						end = start + 1;  // end points from ' ' to end of str

					// ex) a * **b (multiply then pointer)
					if(row>1 && !strcmp(tokens[row - 2], "*") && (all_star(tokens[row - 1]) == 1)){  // mul with multiple pointer
						strncat(tokens[row - 1], start, end - start);  // attach *
						row--;  // dec row to complete rest of token
					}
					
					// ex) a*b(multiply)
					else if(is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1]) == 1){  // prev token is [0-9a-zA-Z] and op == *
						strncat(tokens[row], start, end - start);  // attach * to token
					}

					// ex) ,*b (pointer)
					else if(strpbrk(tokens[row - 1], op) != NULL){  // if prev token has op	
						strncat(tokens[row] , start, end - start);  // attatch * to token
							
					}
					else
						strncat(tokens[row], start, end - start);  // attach end to token

					start += (end - start);  // start point next token
				}

			 	else if(row == 0)  // first token
				{
					if((end = strpbrk(start + 1, op)) == NULL){  // no op after current *
						strncat(tokens[row], start, 1);  // attach * to tokens
						start += 1;  // inc start
					}
					else{  // there's op after current *
						while(start < end){
							// 이전 문자가 공백이고 현재 row의 마지막 문자가 [0-9a-zA-Z]
							if(*(start - 1) == ' ' && is_character(tokens[row][strlen(tokens[row]) - 1]))  // c
								return false;
							else if(*start != ' ')  // 공백이 아닐 때까지 문자열 붙이기
								strncat(tokens[row], start, 1);
							start++;  // 다음 문자 가리키기
						}
						if(all_star(tokens[row]))  // row 문자열이 모두 *이면 row 감소
							row--;
						
					}
				}
			}
			else if(*end == '(')  // 연산자가 ( 라면
			{
				lcount = 0;
				rcount = 0;
				// 첫 토큰이 아니고 이전 토큰이 &이거나, 이전 토큰이 *이라면
				// ex) &(var), *(foo)
				if(row>0 && (strcmp(tokens[row - 1],"&") == 0 || strcmp(tokens[row - 1], "*") == 0)){
					while(*(end + lcount + 1) == '(')  // ( 개수만큼 lcount 증가
						lcount++;
					start += lcount;  // ( 건너뛰기

					end = strpbrk(start + 1, ")");  // end가 ) 가리킴

					if(end == NULL)  // )가 없으면 오답
						return false;
					else{
						while(*(end + rcount +1) == ')')  // ) 개수만큼 rcount 증가
							rcount++;
						end += rcount;  // ) 건너뛰기

						if(lcount != rcount)  // (와 ) 개수가 다르면 오답
							return false;

						// 첫 번째 토큰이 아니고 이전 이전 토큰의 마지막 문자가 [0-9a-zA-Z]가 아니거나 첫 토큰이면
						if( (row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1])) || row == 1){ 
							strncat(tokens[row - 1], start + 1, end - start - rcount - 1);  // 이전 토큰에 ( ) 안의 문자 넣기
							row--;  // row 감소
							start = end + 1;  // ( ... ) 뒤 가리키기
						}
						else{
							strncat(tokens[row], start, 1);  // 현재 토큰에 문자 붙이기
							start += 1;  // start 증가
						}
					}
						
				}
				else{  // 일반 키워드 이후의 ( 의 경우. ex) creat(...), open(...)
					strncat(tokens[row], start, 1);  // 토큰에 ( 추가
					start += 1;  // 다음 문자 가리키기
				}

			}
			else if(*end == '\"')   // 연산자가 "라면
			{
				end = strpbrk(start + 1, "\"");  // 다음 " 가리키기
				
				if(end == NULL)  // "가 없다면 오답
					return false;

				else{
					strncat(tokens[row], start, end - start + 1);  // 현재 토큰에 "..." 사이 문자열 붙이기
					start = end + 1;  // "..." 건너뛰기
				}

			}

			else{  // ',', '|', ')', 
				// ex) a++ ++ +b
				if(row > 0 && !strcmp(tokens[row - 1], "++"))  // 이전 토큰이 ++가 아니면 오답
					return false;

				// ex) a-- -- -b
				if(row > 0 && !strcmp(tokens[row - 1], "--"))  // 이전 토큰이 --가 아니면 오답
					return false;
	
				strncat(tokens[row], start, 1);  // 토큰에 연산자 추가
				start += 1;  // 다음 문자 가리키기
				
				// 방금 추가한게 - + ++ 이면
				// ex) -a or a, -b
				if(!strcmp(tokens[row], "-") || !strcmp(tokens[row], "+") || !strcmp(tokens[row], "--") || !strcmp(tokens[row], "++")){


					// ex) -a or -a+b
					if(row == 0)
						row--;

					// ex) a+b = -c
					else if(!is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])){
						// 이전 토큰이 ++, --를 안 가지고 있으면 row 감소
						if(strstr(tokens[row - 1], "++") == NULL && strstr(tokens[row - 1], "--") == NULL)
							row--;
					}
				}
			}
		}
		else{  // str은 op로 시작하지 않음
			if(all_star(tokens[row - 1]) && row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1]))  // prev token is 1 or more *s && prev prev token's last char is not character
				row--;				

			if(all_star(tokens[row - 1]) && row == 1)  // prev token is 1 or more *s && current row = 1
				row--;	

			// 현재 토큰에 키워드 추가
			// i는 읽은 키워드 길이
			for(i = 0; i < end - start; i++){  // end - start is keyword in front of op. ex) lseek, creat
				if(i > 0 && *(start + i) == '.'){  // 현재 글자가 .이라면
					strncat(tokens[row], start + i, 1);  // 현재 토큰에 .을 추가한다

					while( *(start + i +1) == ' ' && i< end - start )  // 다음 글자가 공백이면
						i++;  // space 건너뛰기
				}
				else if(start[i] == ' '){  // 현재 글자가 space라면
					while(start[i] == ' ')  // space 가 아닌 글자까지 i 증가
						i++;
					break;
				}
				else
					strncat(tokens[row], start + i, 1);  // 현재 토큰에 문자를 추가한다
			}

			if(start[0] == ' '){  // start가 space로 시작하면
				start += i;  // 스페이스 건너뛰기
				continue;
			}
			start += i;  // 읽은 키워드 만큼 인덱스 증가
		}
			
		strcpy(tokens[row], ltrim(rtrim(tokens[row])));  // 위에서 읽은 토큰의 좌우 공백 제거

		// 두번째 이상 토큰이고 현재 토큰의 마지막 글자가 [0-9a-Z]이고
		// && 이전 토큰이 gcc나 datatype or 이전 토큰의 마지막 글자가 [0-9a-Z] or .
		if(row > 0 && is_character(tokens[row][strlen(tokens[row]) - 1]) 
				&& (is_typeStatement(tokens[row - 1]) == 2 
					|| is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])
					|| tokens[row - 1][strlen(tokens[row - 1]) - 1] == '.' ) ){

			if(row > 1 && strcmp(tokens[row - 2],"(") == 0)  // 이전 이전 토큰이 ( 라면
			{	
				// 이전 토큰이 struct나 unsigned가 아니면 오답
				if(strcmp(tokens[row - 1], "struct") != 0 && strcmp(tokens[row - 1],"unsigned") != 0)
					return false;
			}
			// 두번째 토큰이고 현재 토큰의 마지막 문자가 [0-9a-zA-Z]라면
			else if(row == 1 && is_character(tokens[row][strlen(tokens[row]) - 1])) {
				// 첫 토큰이 extern, unsigned, datatype, gcc가 아니면 오답
				if(strcmp(tokens[0], "extern") != 0 && strcmp(tokens[0], "unsigned") != 0 && is_typeStatement(tokens[0]) != 2)	
					return false;
			}
			// 3번째 이상 토큰이고 이전 토큰이 gcc, datatype이면
			else if(row > 1 && is_typeStatement(tokens[row - 1]) == 2){
				// 이전 이전 토큰이 unsigned, extern이 아니면 오답
				if(strcmp(tokens[row - 2], "unsigned") != 0 && strcmp(tokens[row - 2], "extern") != 0)
					return false;
			}
			
		}

		if((row == 0 && !strcmp(tokens[row], "gcc")) ){  // 답안의 첫 번째 토큰이 gcc라면
			clear_tokens(tokens);  // tokens 초기화
			strcpy(tokens[0], str);	 // 첫 번째 토큰에 답안 전체를 저장
			return 1;  // 성공 리턴
		} 

		row++;  // 다음 토큰 저장 준비
	}
	// str 쪼개서 tokens에 저장 끝

	// 마지막 토큰이 *로만 이루어져 있고 토큰이 2개 이상이고 뒤에서 2번째 토큰이 [0-9a-Z]가 아니면
	// a.* 뭐 이런 답이 있나
	if(all_star(tokens[row - 1]) && row > 1 && !is_character(tokens[row - 2][strlen(tokens[row - 2]) - 1]))  
		row--;				
	// 토큰이 총 2개이고 마지막 문자가 *로 이루어져 있으면
	if(all_star(tokens[row - 1]) && row == 1)   
		row--;	

	// creat(fname, S_IRUSR | ... )은 여기에 안들어감.
	for(i = 0; i < strlen(start); i++)  // start 문자열 분석
	{
		if(start[i] == ' ')  // 현재 문자가 공백이면
		{
			while(start[i] == ' ')  // 공백 건너뛰기
				i++;
			if(start[0]==' ') {  // 첫 문자가 공백이면
				start += i;  // 공백 건너뛰기
				i = 0;  // i 초기화
			}
			else
				row++;  // 행 증가
			
			i--;  // i 감소
		} 
		else  // 현재 문자가 공백이 아니면
		{
			strncat(tokens[row], start + i, 1);  // 토큰에 start부터 현재 문자 붙이기
			if( start[i] == '.' && i<strlen(start)){  // 현재 문자가 .이고 현재 문자가 start 안이라면
				while(start[i + 1] == ' ' && i < strlen(start))  // 공백 문자 건너뛰기
					i++;

			}
		}
		strcpy(tokens[row], ltrim(rtrim(tokens[row])));  // 현재 토큰에 좌우 공백 제거

		// 현재 토큰이 lpthread이고 첫 번쨰 토큰이 아니고 이전 토큰이 -라면
		if(!strcmp(tokens[row], "lpthread") && row > 0 && !strcmp(tokens[row - 1], "-")){ 
			strcat(tokens[row - 1], tokens[row]);  // 이전 토큰에 현재 토큰 붙이기
			memset(tokens[row], 0, sizeof(tokens[row]));  // 현재 토큰 0으로 초기화
			row--;  // row 감소
		}
		// 첫 토큰이 아니고 현재 토큰의 마지막 문자가 [0-9a-zA-Z]이거나
		// 이전 토큰이 datatype이거나 이전 토큰의 마지막 문자가 [0-9a-zA-Z]거나
		// 이전 토큰의 마지막 문자가 .이라면
	 	else if(row > 0 && is_character(tokens[row][strlen(tokens[row]) - 1]) 
				&& (is_typeStatement(tokens[row - 1]) == 2 
					|| is_character(tokens[row - 1][strlen(tokens[row - 1]) - 1])
					|| tokens[row - 1][strlen(tokens[row - 1]) - 1] == '.') ){
			// 첫 토큰이 아니고 이전 이전 토큰이 ( 이면
			if(row > 1 && strcmp(tokens[row-2],"(") == 0)
			{
				// 이전 토큰이 struct, unsigned가 아니면 오답
				if(strcmp(tokens[row-1], "struct") != 0 && strcmp(tokens[row-1], "unsigned") != 0)
					return false;
			}
			// 두 번째 토큰이고 현재 토큰의 마지막 문자가 [0-9a-zA-Z]라면
			else if(row == 1 && is_character(tokens[row][strlen(tokens[row]) - 1])) {
				// 첫 토큰이 extern, unsigned가 아니면 오답
				if(strcmp(tokens[0], "extern") != 0 && strcmp(tokens[0], "unsigned") != 0 && is_typeStatement(tokens[0]) != 2)	
					return false;
			}
			// 첫 토큰이 아니고 이전 토큰이 datatype이면
			else if(row > 1 && is_typeStatement(tokens[row - 1]) == 2){
				// 이전 이전 토큰이 unsigned, extern이 아니면 오답
				if(strcmp(tokens[row - 2], "unsigned") != 0 && strcmp(tokens[row - 2], "extern") != 0)
					return false;
			}
		} 
	}

	// 토큰이 하나 이상이라면
	if(row > 0)
	{

		// ex) #include <sys/types.h>
		/* 공백 여러 칸을 한 칸으로 바꿈. 없으면 한칸 띄워줌 */
		/* ex1) #include   <test.h> => #include <test.h> */
		/* ex2) #include<test.h> => #include <test.h> */
		if(strcmp(tokens[0], "#include") == 0 || strcmp(tokens[0], "include") == 0 || strcmp(tokens[0], "struct") == 0){ 
			clear_tokens(tokens);  // tokens 초기화
			strcpy(tokens[0], remove_extraspace(str));  // 공백 제거
		}
	}
	// 첫 토큰이 gcc거나 datatype거나 첫 토큰에 extern이 포함된 경우
	if(is_typeStatement(tokens[0]) == 2 || strstr(tokens[0], "extern") != NULL){
		for(i = 1; i < TOKEN_CNT; i++){  // 토큰 수 만큼 반복
			if(strcmp(tokens[i],"") == 0)  // i번쨰 토큰이 공백이면 break;
				break;		       

			if(i != TOKEN_CNT -1 )  // 마지막 토큰이 아니면
				strcat(tokens[0], " ");  // 첫 토큰에 공백 추가
			strcat(tokens[0], tokens[i]);  // 첫 토큰에 현재 토큰 붙이기
			memset(tokens[i], 0, sizeof(tokens[i]));  // i번째 토큰 0으로 초기화
		}
	}
	
	//change ( ' char ' )' a  ->  (char)a
	// p_str : 정답에 아래 형태가 나오면 datatype토큰의 인덱스 저장
	// (datatype)&, (datatype)*, (datatype)), (datatype)(,
	// (datatype)-, (datatype)+, (datatype)[0-9a-Z] 라면
	while((p_str = find_typeSpecifier(tokens)) != -1){  
		if(!reset_tokens(p_str, tokens))  // 정답이 쪼개지지 않으면 오답
			return false;
	}

	//change sizeof ' ( ' record ' ) '-> sizeof(record)
	// p_str : 정답에 struct가 나오면 struct 토큰의 인덱스 저장
	while((p_str = find_typeSpecifier2(tokens)) != -1){  
		if(!reset_tokens(p_str, tokens))  // 정답이 쪼개지지 않으면 오답
			return false;
	}
	
	return true;  // 정답 쪼개기 성공
}

/* score_blank() 에서 호출됨 */
/* 학생의 답안 토큰들로 lcrs 트리를 만든다 */
/* root : std_root, tokens : 답안 토큰들 */
/* idx : tokens의 인덱스로 함수 종료 후엔 노드 개수가 저장됨 */
/* parentheses : 괄호로 감싸진 횟수 */
node *make_tree(node *root, char (*tokens)[MINLEN], int *idx, int parentheses)
{
	node *cur = root;  // 최근에 만든 노드. cur 기준으로 새 노드를 추가함
	node *new;  // 새로 추가 할 노드
	node *saved_operator;  // 연산자 저장
	node *operator;  // 연산자
	int fstart;  // 함수에서 ( ) 안의 첫 노드 생성시 true. true면 new를 자식으로, false면 형제로 추가
	int i;  // 반복문 인덱스

	while(1)	
	{
		if(strcmp(tokens[*idx], "") == 0)  // 모든 토큰 노드를 생성했으면 종료
			break;
	
		if(!strcmp(tokens[*idx], ")"))  // 현재 토큰이 ) 이면 root 리턴하고 종료
			return get_root(cur);

		else if(!strcmp(tokens[*idx], ","))  // 현재 토큰이 , 이면 root 리턴하고 종료
			return get_root(cur);

		else if(!strcmp(tokens[*idx], "("))  // 현재 토큰이 ( 라면
		{
			// function()
			// (가 첫 번째 토큰이 아니고 이전 토큰이 연산자나 ,가 아니면
			if(*idx > 0 && !is_operator(tokens[*idx - 1]) && strcmp(tokens[*idx - 1], ",") != 0){
				fstart = true;  // 매개변수들을 노드로 만들기 시작

				while(1)  // 함수의 매개변수 안 토큰들을, 기준으로 트리 생성
				{
					*idx += 1;  // 다음 토큰 가리키기

					if(!strcmp(tokens[*idx], ")"))  // 매개변수가 없으면 노드 생성 안하고 종료
						break;
					
					new = make_tree(NULL, tokens, idx, parentheses + 1);  // 트리 생성
					
					if(new != NULL){  // 노드가 생성됐고
						if(fstart == true){  // 현재 함수라면
							cur->child_head = new;  // 첫째를 new로 한다
							new->parent = cur;  // 새 노드의 부모를 cur로 한다
	
							fstart = false;  // 함수 끝
						}
						else{
							cur->next = new;  // 현재 노드의 형제로 새 노드 추가
							new->prev = cur;  // 양 방향 추가
						}

						cur = new;  // 이제 cur 노드는 방금 만든 노드
					}

					if(!strcmp(tokens[*idx], ")"))  // 함수 안 매개변수 노드 생성이 끝났다면 종료
						break;
				}
			}
			else{
				*idx += 1;  // 인덱스 증가
	
				new = make_tree(NULL, tokens, idx, parentheses + 1);  // 서브 트리 생성

				if(cur == NULL)  // NULL이면 cur이 가리킴
					cur = new;

				else if(!strcmp(new->name, cur->name)){  // new, cur의 name이 같으면
					// new->name이 아래 연산자면
					if(!strcmp(new->name, "|") || !strcmp(new->name, "||") 
						|| !strcmp(new->name, "&") || !strcmp(new->name, "&&"))
					{
						cur = get_last_child(cur);  // 마지막 child 노드 가리키기

						if(new->child_head != NULL){  // 새 노드의 첫째 노드가 있다면
							new = new->child_head;  // 새 노드는 이전 첫째 노드

							new->parent->child_head = NULL;  // 부모의 첫째 노드 없애기
							new->parent = NULL;  // new 노드의 parent 비우기
							new->prev = cur;  // 새 노드의 이전 노드는 cur
							cur->next = new;  // cur의 다음 노드는 next
						}
					}
					// new->name이 아래 연산자면
					else if(!strcmp(new->name, "+") || !strcmp(new->name, "*"))
					{
						i = 0;  // 인덱스 초기화

						while(1)
						{
							// *idx + i번째 토큰이 공백이면 break;
							if(!strcmp(tokens[*idx + i], ""))
								break;

							// *idx + i번째 토큰이 연산자이고 )가 있다면 break;
							if(is_operator(tokens[*idx + i]) && strcmp(tokens[*idx + i], ")") != 0)
								break;

							i++;  // 인덱스 증가
						}
						
						// *idx+i 번째 토큰의 연산자 우선 순위가 new->name의 연산자 우선순위보다 낮다면
						if(get_precedence(tokens[*idx + i]) < get_precedence(new->name))
						{
							cur = get_last_child(cur);  // cur은 마지막 노드
							cur->next = new;  // 마지막 노드의 다음에 new 추가
							new->prev = cur;  // 새 노드의 이전 노드는 cur
							cur = new;  // cur이 새 노드를 가리킴
						}
						else  // new->name의 연산자 우선 순위가 높다면
						{
							cur = get_last_child(cur); // cur은 마지막 노드

							if(new->child_head != NULL){  // 첫쨰가 있다면
								new = new->child_head;  // new는 기존 첫째 가리키기

								new->parent->child_head = NULL;  // 새 노드의 부모의 첫째 지우기
								new->parent = NULL;  // 부모 지우기
								new->prev = cur;  // 새 노드의 이전 노드를 마지막 노드로
								cur->next = new;  // 마지막 노드의 다음 노드를 new로
							}
						}
					}
					else{
						cur = get_last_child(cur);  // cur은 마지막 노드
						cur->next = new;  // 마지막 노드의 next를 새 노드로
						new->prev = cur;  // 새 노드의 이전 노드를 cur 노드로
						cur = new;  // cur은 마지막 노드를 가리킴
					}
				}
	
				else
				{
					cur = get_last_child(cur);  // cur은 마지막 노드

					cur->next = new;  // 마지막 노드의 next를 새 노드로
					new->prev = cur;  // 새 노드의 이전 노드를 cur 노드로
	
					cur = new;  // cur은 마지막 노드를 가리킴
				}
			}
		}
		else if(is_operator(tokens[*idx]))  // 현재 토큰이 연산자라면
		{
			// 현재 토큰이 아래 연산자 중 하나라면
			if(!strcmp(tokens[*idx], "||") || !strcmp(tokens[*idx], "&&")
					|| !strcmp(tokens[*idx], "|") || !strcmp(tokens[*idx], "&") 
					|| !strcmp(tokens[*idx], "+") || !strcmp(tokens[*idx], "*"))
			{
				// cur이 연산자이고 현재 토큰과 일치하면
				if(is_operator(cur->name) == true && !strcmp(cur->name, tokens[*idx]))
					operator = cur;  // op노드가 cur 가리키기
		
				else  // 일치하지 않으면
				{
					new = create_node(tokens[*idx], parentheses);  // 노드 생성
					operator = get_most_high_precedence_node(cur, new);  // 가장 높은 우선순위 연산자

					if(operator->parent == NULL && operator->prev == NULL){  // 연산자의 부모, 이전 노드가 없으면

						if(get_precedence(operator->name) < get_precedence(new->name)){  // op가 new보다 우선순위가 낮으면
							cur = insert_node(operator, new);  // 노드 삽입
						}
						// op의 우선 순위가 높으면
						else if(get_precedence(operator->name) > get_precedence(new->name))
						{
							if(operator->child_head != NULL){  // 첫째 노드가 있다면
								operator = get_last_child(operator);  // 마지막 노드 가져와서
								cur = insert_node(operator, new);  // new 삽입
							}
						}
						else  // 우선 순위가 같다면
						{
							operator = cur;  // cur 가리키기
	
							while(1)
							{
								// op가 연산자이고 현재 토큰과 같다면 break;
								if(is_operator(operator->name) == true && !strcmp(operator->name, tokens[*idx]))
									break;
						
								if(operator->prev != NULL)  // 이전 노드가 있다면 가리키기
									operator = operator->prev;
								else  // 없으면 break;
									break;
							}

							if(strcmp(operator->name, tokens[*idx]) != 0)  // op와 현재 토큰이 다르면
								operator = operator->parent;  // 부모 노드 가리키기

							if(operator != NULL){  // op 노드가 존재하면
								if(!strcmp(operator->name, tokens[*idx]))  // op와 현재 토큰이 같으면 cur이 가리키기
									cur = operator;
							}
						}
					}

					else
						cur = insert_node(operator, new);  // 새로 삽입한 노드 가리키기
				}

			}
			else  
			{
				new = create_node(tokens[*idx], parentheses);  // 새 노드 생성

				if(cur == NULL)  // cur이 없으면 가리키기
					cur = new;

				else  // cur이 있으면
				{
					operator = get_most_high_precedence_node(cur, new);  // cur와 new의 제일 높은 우선순위 노드

					if(operator->parentheses > new->parentheses)  // op가 new보다 ()가 많으면
						cur = insert_node(operator, new);  // 노드 삽입

					else if(operator->parent == NULL && operator->prev ==  NULL){  // op의 부모, 이전 노드가 없으면
					
						if(get_precedence(operator->name) > get_precedence(new->name))  // op의 우선순위가 new보다 높으면
						{
							if(operator->child_head != NULL){  // 첫째 노드가 있다면
	
								operator = get_last_child(operator);  // 마지막 노드 가리키기
								cur = insert_node(operator, new);  // 노드 삽입
							}
						}
					
						else	// op의 우선순위가 new보다 낮으면
							cur = insert_node(operator, new);  // 노드 삽입
					}
	
					else
						cur = insert_node(operator, new);  // 노드 삽입
				}
			}
		}
		else // '(', ')', ',', 연산자가 아닌 토큰
		{
			new = create_node(tokens[*idx], parentheses);  // 현재 토큰으로 노드 생성

			if(cur == NULL)  // 현재 가리키는 노드가 없으면 새 노드를 가리킴
				cur = new;

			else if(cur->child_head == NULL){  // 첫째가 없으면
				cur->child_head = new;  // new가 첫째
				new->parent = cur;  // new의 부모는 cur 

				cur = new;  // 새 노드 가리키기
			}
			else{  // 첫째가 있으면
				cur = get_last_child(cur);  // 막내 노드 가져오기

				cur->next = new;  // 막내 다음에 new 추가
				new->prev = cur;  // new의 이전은 막내

				cur = new;  // 새 노드 가리키기
			}
		}

		*idx += 1;  // 다음 토큰 가리키기
	}

	return get_root(cur);  // root 노드 리턴
}

/* compare_tree에서 호출됨 */
/* parent의 첫째와 둘째 노드를 바꾼다 */
/* parent : 부모 노드 */
node *change_sibling(node *parent)
{
	node *tmp;
	
	// tmp is first node among siblings
	tmp = parent->child_head;

	// change child_head from first child to second child
	parent->child_head = parent->child_head->next;
	// set parent to new child_head
	parent->child_head->parent = parent;
	// delete new child_head's prev node
	parent->child_head->prev = NULL;

	// set old_child_head to new_child_head's next
	parent->child_head->next = tmp;
	// set old_child's prev to new_child_head
	parent->child_head->next->prev = parent->child_head;
	// delete old_child_head's next
	parent->child_head->next->next = NULL;
	// delete old_child head's parent
	parent->child_head->next->parent = NULL;		

	return parent;
}

/* make_tree()에서 호출됨 */
/* name : 토큰에 저장된 문자열, parentheses : 괄호 여부 */
node *create_node(char *name, int parentheses)
{
	node *new;  // 새로 만들 노드의 포인터

	new = (node *)malloc(sizeof(node));  // 메모리 할당
	new->name = (char *)malloc(sizeof(char) * (strlen(name) + 1));  // 토큰 내용 저장을 위한 메모리 할당
	strcpy(new->name, name);  // 노드에 토큰 내용 복사

	new->parentheses = parentheses;  // 괄호 여부 복사
	new->parent = NULL;  
	new->child_head = NULL;
	new->prev = NULL;
	new->next = NULL;

	return new;
}

/* make_tree(), get_high_precedence_node에서 호출됨 */
/* op의 우선순위 반환 */
int get_precedence(char *op)
{
	int i;

	for(i = 2; i < OPERATOR_CNT; i++){  // 모든 연산자를 순회하며
		if(!strcmp(operators[i].operator, op))  // op가 연산자면
			return operators[i].precedence;  // 연산자의 우선순위 반환
	}
	return false;
}

/* make_tree()에서 호출됨 */
/* op : 확인할 문자 */
int is_operator(char *op)
{
	int i;

	for(i = 0; i < OPERATOR_CNT; i++)
	{
		if(operators[i].operator == NULL)  // 모든 연산자를 확인했으면 종료
			break;
		if(!strcmp(operators[i].operator, op)){  // op가 i번째 연산자라면
			return true;  // 연산자 맞음
		}
	}

	return false; // 연산자 아님
}

/* cur노드 출력하기 */
void print(node *cur)
{
	if(cur->child_head != NULL){  // 첫째 노드가 있다면
		print(cur->child_head);  // 첫째 출력
		printf("\n");
	}

	if(cur->next != NULL){  // 다음 노드가 있다면
		print(cur->next);  // 다음 노드 출력
		printf("\t");
	}
	printf("%s", cur->name);  // name 출력
}

/* make_tree() 에서 호출됨 */
/* cur의 형제 중 첫째로 이동해서 부모 노드 리턴 */
/* 자식들이 피 연산자이면 부모인 연산자를 리턴 */
/* cur : 트리의 특정 노드 */
node *get_operator(node *cur)
{
	if(cur == NULL)  // cur이 NULL이면 NULL 리턴
		return cur;

	if(cur->prev != NULL)  // 이전 노드가 있다면
		while(cur->prev != NULL)  // 가장 왼쪽 노드로 이동
			cur = cur->prev;

	return cur->parent;  // 부모 노드 리턴
}

/* make_tree() 에서 호출됨 */
/* cur이 속한 트리의 root 반환 */
/* cur : 트리의 특정 노드 */
node *get_root(node *cur)
{
	if(cur == NULL)  // cur이 NULL이면 NULL 리턴
		return cur;

	while(cur->prev != NULL)  // 형제 중 가장 왼쪽 노드로 이동
		cur = cur->prev;

	if(cur->parent != NULL)  // 부모가 있다면 부모에게 get_root
		cur = get_root(cur->parent);

	return cur;
}

/* get_most_high_precedence_node() 에서 호출됨 */
node *get_high_precedence_node(node *cur, node *new)
{
	if(is_operator(cur->name))  // cur이 연산자이면
		if(get_precedence(cur->name) < get_precedence(new->name))  // cur의 연산자 우선 순위가 낮으면 리턴
			return cur;

	if(cur->prev != NULL){  // cur이 이전 노드가 있다면
		while(cur->prev != NULL){  // 첫번째 노드로 이동
			cur = cur->prev;
			
			return get_high_precedence_node(cur, new);  // 첫 번째 노드와 new의 비교값 리턴
		}


		if(cur->parent != NULL)  // 부모가 있다면
			return get_high_precedence_node(cur->parent, new);  // 부모와 new의 비교값 리턴
	}

	if(cur->parent == NULL)  // 부모가 없다면
		return cur;  // cur 리턴
}

/* make_tree()에서 호출됨 */
/* cur와 new 노드가 있는 tree의 가장 높은 연산자 노드 리턴 */
node *get_most_high_precedence_node(node *cur, node *new)
{
	node *operator = get_high_precedence_node(cur, new);  // cur와 new 중 높은 연산자 리턴
	node *saved_operator = operator;  // 연산자 저장

	while(1)
	{
		if(saved_operator->parent == NULL)  // 연산자의 부모가 없으면 break;
			break;

		if(saved_operator->prev != NULL)  // 연산자의 이전 노드가 있으면
			operator = get_high_precedence_node(saved_operator->prev, new);  // 이전 노드와 new 비교

		else if(saved_operator->parent != NULL)  // 연산자의 부모가 있으면
			operator = get_high_precedence_node(saved_operator->parent, new);  // 부모와 new 비교

		saved_operator = operator;  // 높은 연산자 저장
	}
	
	return saved_operator;  // 가장 높은 연산자 리턴
}

/* make_tree()에서 호출됨 */
/* old의 prev 자리에 new를 삽입한다 */
node *insert_node(node *old, node *new)
{
	if(old->prev != NULL){  // old의 이전 노드가 있다면
		new->prev = old->prev;  // new의 prev를 old의 prev로
		old->prev->next = new;  // old의 이전 노드의 다음 노드를 new로
		old->prev = NULL;  // old의 이전 노드 없애기
	}

	new->child_head = old;  // 새 노드의 첫 째는 old
	old->parent = new;  // old의 부모를 new로 한다

	return new;
}

/* make_tree()에서 호출됨 */
/* 마지막 노드 리턴 */
node *get_last_child(node *cur)
{
	if(cur->child_head != NULL)  // 첫 쨰가 있다면
		cur = cur->child_head;  // 첫째 가리키기

	while(cur->next != NULL)  // 다음 노드가 있다면
		cur = cur->next;  // 다음 노드 가리키기

	return cur;
}

/* compare_tree()에서 호출됨 */
/* return : cur을 제외한 형제의 노드 수 */
int get_sibling_cnt(node *cur)
{
	int i = 0;

	while(cur->prev != NULL)  // 첫째 노드로 이동
		cur = cur->prev;

	while(cur->next != NULL){  // 다음 노드가 있으면 next로 이동하며 개수 증가
		cur = cur->next;
		i++;
	}

	return i;
}

/* node를 free한다 */
void free_node(node *cur)
{
	if(cur->child_head != NULL)  // 재귀로 첫째 노드 free
		free_node(cur->child_head);

	if(cur->next != NULL)  // 재귀로 다음 노드 free
		free_node(cur->next);

	if(cur != NULL){  // 현재 노드가 있다면
		cur->prev = NULL;  // prev 지우기
		cur->next = NULL;  // next 지우기
		cur->parent = NULL;  // parent 지우기
		cur->child_head = NULL;  // child_head 지우기
		free(cur);  // free
	}
}

/* called in make_tokens() */
/* return : is [0-9a-zA-Z] */
/* c : char in s_ans */
int is_character(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/* called in make_tokens */
/* check str is Statement */
/* 0 : wrong answer */
/* 1 : s_answer is normal answer */
/* 2 : s_answer starts with gcc | or datatype */
/* str : student's answer ex) lseek(fd, 15000, 0) */
int is_typeStatement(char *str)
{ 
	char *start;  // index for str
	char str2[BUFLEN] = {0};  // save str with no space
	char tmp[BUFLEN] = {0}; 
	char tmp2[BUFLEN] = {0};  // if student's answer has "gcc", save here 
	int i;	 
	
	start = str;  // points str's head
	strncpy(str2,str,strlen(str));  // copy str to str2
	remove_space(str2);  // remove space in str. ex) lseek(fd,15000,0)

	while(start[0] == ' ')  // make start points non-space char of str
		start += 1;  // ++ index

	if(strstr(str2, "gcc") != NULL)  // str2 has "gcc"
	{
		strncpy(tmp2, start, strlen("gcc"));  // save "gcc" to 3 byte in start
		if(strcmp(tmp2,"gcc") != 0)  // if student's answer doesn't start with gcc
			return 0;  // return 0
		else           // student's answer starts with gcc
			return 2;  // return 2
	}
	
	for(i = 0; i < DATATYPE_SIZE; i++)  // i = 0 ~ < 35
	{
		if(strstr(str2,datatype[i]) != NULL)  // if s_answer has i'th datatype
		{	
			strncpy(tmp, str2, strlen(datatype[i]));  // save first part
			strncpy(tmp2, start, strlen(datatype[i]));  // save first part
			
			if(strcmp(tmp, datatype[i]) == 0)  // if str2 starts with datatype[i]
				if(strcmp(tmp, tmp2) != 0)  // if str and str2's first part is diff
					return 0;  // wrong answer. s_answer has space in datatype
				else
					return 2;  // s_answer starts with datatype
		}

	}
	return 1;  // normal answer

}

/* make_tokens()에서 호출됨 */
/* 답안에 (datatype) 가 있으면 datatype이 몇 번째 토큰인지 리턴 */
/* tokens : 답안을 쪼갠 토큰들 */
int find_typeSpecifier(char tokens[TOKEN_CNT][MINLEN]) 
{
	int i, j;

	for(i = 0; i < TOKEN_CNT; i++)
	{
		for(j = 0; j < DATATYPE_SIZE; j++)
		{
			// 토큰 중간에 datatype이 나온다면
			if(strstr(tokens[i], datatype[j]) != NULL && i > 0)
			{
				// datatype이 (와 )로 감싸지고
				// (datatype)&, (datatype)*, (datatype)), (datatype)(,
				// (datatype)-, (datatype)+, (datatype)[0-9a-Z] 라면
				if(!strcmp(tokens[i - 1], "(") && !strcmp(tokens[i + 1], ")") 
						&& (tokens[i + 2][0] == '&' || tokens[i + 2][0] == '*' 
							|| tokens[i + 2][0] == ')' || tokens[i + 2][0] == '(' 
							|| tokens[i + 2][0] == '-' || tokens[i + 2][0] == '+' 
							|| is_character(tokens[i + 2][0])))  
					return i;  // datatype 토큰의 인덱스 리턴
			}
		}
	}
	return -1;  // 토큰 중간에 datatype이 없음
}

/* make_tokens()에서 호출됨 */
/* tokens : 답안을 쪼갠 토큰들 */
int find_typeSpecifier2(char tokens[TOKEN_CNT][MINLEN]) 
{
    int i, j;

   
    for(i = 0; i < TOKEN_CNT; i++)
    {
        for(j = 0; j < DATATYPE_SIZE; j++)
        {
			// 토큰 중 struct가 있고 struct 토큰이 마지막 토큰이 아니고 struct의 다음 토큰이 [0-9a-Z]로 끝난다면
			// ex) struct a {...}; 이런 형태인듯
            if(!strcmp(tokens[i], "struct") && (i+1) <= TOKEN_CNT && is_character(tokens[i + 1][strlen(tokens[i + 1]) - 1]))  
                    return i;  // struct 토큰의 인덱스 리턴
        }
    }
    return -1;  // 답에 struct가 없음
}

/* make_tokens()에서 호출됨 */
/* return */
/* 0 : str에 *이 아닌 문자가 있음 */
/* 1 : str이 *로만 이루어짐 */
/* str : prev token */
int all_star(char *str)
{
	int i;  // index
	int length= strlen(str);  // len of token
	
 	if(length == 0)	// no *
		return 0;
	
	for(i = 0; i < length; i++)
		if(str[i] != '*')  // not * char exists
			return 0;
	return 1;  // only * there

}

/* str의 i번째 문자가 char이면 1 리턴 */
int all_character(char *str)
{
	int i;

	for(i = 0; i < strlen(str); i++)  // 문자열 순회
		if(is_character(str[i]))  // i번째 문자가 문자이면
			return 1;  // 1 리턴
	return 0;  // 아니면 0 리턴
	
}

/* make_tokens() 에서 호출됨 */
/* start : tokens에서 datatype 또는 struct의 인덱스 */
/* tokens : 정답을 쪼갠 토큰들 */
int reset_tokens(int start, char tokens[TOKEN_CNT][MINLEN]) 
{
	int i;
	int j = start - 1;
	int lcount = 0, rcount = 0;  // (와 ) 개수
	int sub_lcount = 0, sub_rcount = 0;  // sub (와 ) 개수

	if(start > -1){
		if(!strcmp(tokens[start], "struct")) {  // 현재 토큰이 struct이면
			strcat(tokens[start], " ");  // 공백 붙이기
			strcat(tokens[start], tokens[start+1]);  // 현재 토큰에 다음 토큰 붙이기  

			for(i = start + 1; i < TOKEN_CNT - 1; i++){ 
				strcpy(tokens[i], tokens[i + 1]);  // 다음 토큰을 현재 토큰으로 복사
				memset(tokens[i + 1], 0, sizeof(tokens[0]));  // 다음 토큰 초기화
			}
		}
		// 현재 토큰이 unsigned이고 다음 토큰이 )가 아니면
		else if(!strcmp(tokens[start], "unsigned") && strcmp(tokens[start+1], ")") != 0) {		
			strcat(tokens[start], " ");  // 공백 붙이기
			strcat(tokens[start], tokens[start + 1]);  // 다음 토큰을 현재 토큰에 붙이기     
			strcat(tokens[start], tokens[start + 2]);  // 다다음 토큰을 현재 토큰에 붙이기

			for(i = start + 1; i < TOKEN_CNT - 1; i++){
				strcpy(tokens[i], tokens[i + 1]);  // 다음 토큰을 현재 토큰으로 복사
				memset(tokens[i + 1], 0, sizeof(tokens[0]));  // 다음 토큰 초기화
			}
		}

     		j = start + 1;  // 다음 토큰 인덱스 저장
        	while(!strcmp(tokens[j], ")")){  // 다음 토큰이 )이면
                	rcount ++;  // ) 개수 증가
                	if(j==TOKEN_CNT)  // 마지막 토큰 수이면 break;
                        	break;
                	j++;  // j 증가
        	}
	
		j = start - 1;  // 이전 토큰 인덱스 저장
		while(!strcmp(tokens[j], "(")){  // 이전 토큰이 (이면
        	        lcount ++;  // ( 개수 증가
                	if(j == 0)  // 0이면 break;
                        	break;
               		j--;  // j 감소
		}
		// j가 0이 아니고 현재 j번째 토큰이 문자이면
		if( (j!=0 && is_character(tokens[j][strlen(tokens[j])-1]) ) || j==0)
			lcount = rcount;  // )의 개수 저장

		if(lcount != rcount )  // (와 ) 개수가 다르면 오답
			return false;
		// start와 lcount가 0보다 크고 ( 이전 토큰이 sizeof이면 정답
		if( (start - lcount) >0 && !strcmp(tokens[start - lcount - 1], "sizeof")){
			return true; 
		}
		// 현재 토큰이 unsigned, struct이고 다음 토큰이 )이면
		else if((!strcmp(tokens[start], "unsigned") || !strcmp(tokens[start], "struct")) && strcmp(tokens[start+1], ")")) {		
			strcat(tokens[start - lcount], tokens[start]);  // ( 이전 토큰에 현재 토큰 붙이기
			strcat(tokens[start - lcount], tokens[start + 1]);  // 다음 토큰도 붙이기
			strcpy(tokens[start - lcount + 1], tokens[start + rcount]);  // ) 후 토큰을 그 다음에 붙이기
		 
			for(int i = start - lcount + 1; i < TOKEN_CNT - lcount -rcount; i++) {
				strcpy(tokens[i], tokens[i + lcount + rcount]);  // 토큰들을 앞으로 당기기
				memset(tokens[i + lcount + rcount], 0, sizeof(tokens[0]));  // 0으로 초기화
			}


		}
 		else{
			if(tokens[start + 2][0] == '('){  // 다다음 토큰이 (이면
				j = start + 2;  // ( 다음 가리키기
				while(!strcmp(tokens[j], "(")){  // j 토큰이 (이면
					sub_lcount++;  // sub ( 개수 증가
					j++;
				} 	
				if(!strcmp(tokens[j + 1],")")){  // j 토큰이 )이면
					j = j + 1;  // 다음 가리키기
					while(!strcmp(tokens[j], ")")){  // 토큰이 )이면
						sub_rcount++;  // sub ) 개수 증가
						j++;
					}
				}
				else 
					return false;  // 오답

				if(sub_lcount != sub_rcount)  // (, ) 개수가 다르면 오답
					return false;
				
				strcpy(tokens[start + 2], tokens[start + 2 + sub_lcount]);  // start+2 자리에 () 안 토큰 복사	
				for(int i = start + 3; i<TOKEN_CNT; i++)
					memset(tokens[i], 0, sizeof(tokens[0]));  // 0으로 초기화

			}
			strcat(tokens[start - lcount], tokens[start]);  // 토큰 이동
			strcat(tokens[start - lcount], tokens[start + 1]);  // 토큰 이동
			strcat(tokens[start - lcount], tokens[start + rcount + 1]);  // 토큰 이동
		 
			for(int i = start - lcount + 1; i < TOKEN_CNT - lcount -rcount -1; i++) {
				strcpy(tokens[i], tokens[i + lcount + rcount +1]);  // 토큰 당기기
				memset(tokens[i + lcount + rcount + 1], 0, sizeof(tokens[0]));  // 0으로 초기화

			}
		}
	}
	return true;
}

/* called in make_tokens */
/* init 0 */ 
void clear_tokens(char tokens[TOKEN_CNT][MINLEN])
{
	int i;

	for(i = 0; i < TOKEN_CNT; i++)
		memset(tokens[i], 0, sizeof(tokens[i]));  // token을 0으로 초기화
}

/* called in score_blank() */
/* remove right spaces */
/* _str : student's answer */
char *rtrim(char *_str)
{
	char tmp[BUFLEN];  // tmp for _str
	char *end;  // index points tmp's tail

	strcpy(tmp, _str);  // copy
	end = tmp + strlen(tmp) - 1;  // point last char in _str
	while(end != _str && isspace(*end))  // end points first space after answer 
		--end;  // -- index

	*(end + 1) = '\0';  // replace space to \0
	_str = tmp;  // save
	return _str;  // return without no space after chars
}

/* called in score_blank() */
/* remove left spaces */
/* _str : student's answer */
char *ltrim(char *_str)
{
	char *start = _str;  // points _str's head

	while(*start != '\0' && isspace(*start))  // start points last space before answer
		++start;  // ++ index
	_str = start;  // save
	return _str;  // return without no space before chars
}

/* make_tokens()에서 호출됨 */
/* 공백 여러 칸을 한 칸으로 바꿈. 없으면 한칸 띄워줌 */
/* ex1) #include   <test.h> => #include <test.h> */
/* ex2) #include<test.h> => #include <test.h> */
/* str : #include or include */
char* remove_extraspace(char *str)
{
	int i;  // 반복문 인덱스
	char *str2 = (char*)malloc(sizeof(char) * BUFLEN);  // 
	char *start, *end;
	char temp[BUFLEN] = "";
	int position;

	// #include<test.h>를 #include <test.h> 형태로 바꾸기
	if(strstr(str,"include<")!=NULL){  // str에 include< 가 들어가면
		start = str;  // start는 str을 가리킴
		end = strpbrk(str, "<");  // end는 <을 가리킴
		position = end - start;  // position : include의 길이
	
		strncat(temp, str, position);  // temp에 include 저장
		strcat(temp, " ");  // space 추가
		strncat(temp, str + position, strlen(str) - position + 1);  // 나머지 붙이기

		str = temp;  // str에 저장
	}
	
	for(i = 0; i < strlen(str); i++)
	{
		if(str[i] ==' ')  // str에 공백이 있으면
		{
			if(i == 0 && str[0] ==' ')  // 첫 글자가 공백이면
				while(str[i + 1] == ' ')  // 다음 글자가 공백이 아닐 때까지 i 증가
					i++;	
			else{  
				if(i > 0 && str[i - 1] != ' ')  // 이전 문자가 공백이 아니면
					str2[strlen(str2)] = str[i];  // str2에 현재 글자 옮기기
				while(str[i + 1] == ' ')  // 공백 건너뛰기
					i++;
			} 
		}
		else
			str2[strlen(str2)] = str[i];  // str2에 현재 글자 옮기기
	}

	return str2;  // str2에는 공백이 하나만 들어감
}



/* called in is_typeStatement */
/* remove space in str */
/* str : copy of student's answer */
void remove_space(char *str)
{
	char* i = str;  // points str's head
	char* j = str;  // points str's head
	
	while(*j != 0)  // while in str
	{
		*i = *j++;  // overwrite j'th to i'th
		if(*i != ' ')  // if i'th char is space, overwrite next time
			i++;
	}
	*i = 0;  // str ends with \0
}

/* called in score_blank */
/* check if student's answer is grammarly right abount ( ) */
/* std : student's ans. ex) lseek(fd, 15000, 0) */
/* return : 0(wrong) or 1(right) */
int check_brackets(char *str)
{
	char *start = str;  // point str's head
	int lcount = 0, rcount = 0;
	
	while(1){  // check str
		if((start = strpbrk(start, "()")) != NULL){  // if str has ( or )
			if(*(start) == '(')  // current ch is (
				lcount++;  // ( count ++
			else
				rcount++;  // ) count ++

			start += 1;    // find ( or ) in the rest 		
		}
		else
			break;  // no more ( or ), break
	}

	if(lcount != rcount)  // check ( and ) pair
		return 0;  // wrong answer in grammarly
	else 
		return 1;  // right answer in grammarly
}

/* tokens의 NULL 이 아닌 토큰 개수 리턴 */
int get_token_cnt(char tokens[TOKEN_CNT][MINLEN])
{
	int i;
	
	for(i = 0; i < TOKEN_CNT; i++)  // tokens 반복
		if(!strcmp(tokens[i], ""))  // NULL이면 break;
			break;

	return i;
}
