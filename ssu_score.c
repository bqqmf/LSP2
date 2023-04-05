#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "blank.h"
#include "ssu_score.h"
//add header files

extern struct ssu_scoreTable score_table[QNUM];  // 다른 파일에서 score_table 접근 가능
extern char id_table[SNUM][10];  				 // 다른 파일에서 id_table 접근 가능

struct ssu_scoreTable score_table[QNUM];  // 100개의 ssu_scoreTable 저장
char id_table[SNUM][10];

char stuDir[BUFLEN];  // 학생 디렉토리 경로
char ansDir[BUFLEN];  // 정답 디렉토리 경로
char errorDir[BUFLEN];
char threadFiles[ARGNUM][FILELEN];
char iIDs[ARGNUM][FILELEN];

int eOption = false;
int tOption = false;
int mOption = false;
int iOption = false;

void ssu_score(int argc, char *argv[])
{
	char saved_path[BUFLEN];  // 현재 작업 디렉토리 저장
	int i;					  // for을 위한 인덱스

	
	for(i = 0; i < argc; i++){  	 // 인자들 검사
		if(!strcmp(argv[i], "-h")){  // -h 옵션 사용시 usage 출력
			print_usage();
			return;
		}
	}

	memset(saved_path, 0, BUFLEN);  // 배열을 0으로 초기화

	// ./ssu_score student_dir answer_dir 실행 시
	if(argc >= 3 && strcmp(argv[1], "-i") != 0){  
		strcpy(stuDir, argv[1]);  // 학생 디렉토리 상대 경로 저장
		strcpy(ansDir, argv[2]);  // 정답 디렉토리 상대 경로 저장
	}

	// 이상한 옵션 예외 처리
	if(!check_option(argc, argv))
		exit(1);

	// m, e, t 옵션 말고 i 옵션만 사용하고 학생, 정답 디렉토리가 입력된 경우
	if(!mOption && !eOption && !tOption && iOption 
			&& !strcmp(stuDir, "") && !strcmp(ansDir, "")){
		do_iOption(iIDs);  // i 옵션 실행하고 종료
		return;
	}

	getcwd(saved_path, BUFLEN);  // saved_path에 현재 작업 디렉토리 저장

	// 학생 디렉토리로 디렉토리 변경
	if(chdir(stuDir) < 0){
		fprintf(stderr, "%s doesn't exist\n", stuDir);  // stuDir이 존재하지 않는다면 에러 출력 후 종료
		return;
	}
	// stuDir에 학생 디렉토리의 절대 경로 저장? gdb로 실행해보면서 봐야겠다.
	getcwd(stuDir, BUFLEN);

	// ./ssu_score를 실행한 디렉토리로 다시 돌아오기
	chdir(saved_path);

	// 정답 디렉토리로 디렉토리 변경
	if(chdir(ansDir) < 0){
		fprintf(stderr, "%s doesn't exist\n", ansDir);  // ansDir이 존재하지 않는다면 에러 출력 후 종료
		return;
	}
	// ansDir에 정답 디렉토리의 절대 경로 저장? gdb로 실행해보기.
	getcwd(ansDir, BUFLEN);

	// ./ssu_score를 실행한 디렉토리로 다시 돌아오기
	chdir(saved_path);

	set_scoreTable(ansDir);  // score_table.csv를 읽어 구조체 배열 score_table에 저장
	set_idTable(stuDir);	 // id 테이블 생성

	// -m 입력 시 수행
	if(mOption)
		do_mOption();

	printf("grading student's test papers..\n");
	score_students();  // 학생들 점수 매기기

	// -i 입력 시 수행
	if(iOption)
		do_iOption(iIDs);

	return;
}

int check_option(int argc, char *argv[])
{
	int i, j, k;
	int c;
	int exist = 0;

	while((c = getopt(argc, argv, "e:thmi")) != -1)
	{
		switch(c){
			case 'e':
				eOption = true;
				strcpy(errorDir, optarg);

				if(access(errorDir, F_OK) < 0)
					mkdir(errorDir, 0755);
				else{
					rmdirs(errorDir);
					mkdir(errorDir, 0755);
				}
				break;
			case 't':
				tOption = true;
				i = optind;
				j = 0;

				while(i < argc && argv[i][0] != '-'){

					if(j >= ARGNUM)
						printf("Maximum Number of Argument Exceeded.  :: %s\n", argv[i]);
					else{
						strcpy(threadFiles[j], argv[i]);
					}
					i++; 
					j++;
				}
				break;
			case 'm':
				mOption = true;
				break;

			case 'i':
				iOption = true;
				i = optind;
				j = 0;

				while(i < argc && argv[i][0] != '-'){
					if(j >= ARGNUM)
						printf("Maximum Number of Argument Exceeded. :: %s\n", argv[i]);
					else
						strcpy(iIDs[j], argv[i]);
					i++;
					j++;
				}
				break;

			case '?':
				printf("Unkown option %c\n", optopt);
				return false;
		}
	}

	return true;
}

void do_iOption(char (*ids)[FILELEN])
{
	FILE *fp;
	char tmp[BUFLEN];
	char qname[QNUM][FILELEN];
	char *p, *id;
	int i, j;
	char first, exist;

	if((fp = fopen("./score.csv", "r")) == NULL){
		fprintf(stderr, "score.csv file doesn't exist\n");
		return;
	}

	// get qnames
	i = 0;
	fscanf(fp, "%s\n", tmp);
	strcpy(qname[i++], strtok(tmp, ","));

	while((p = strtok(NULL, ",")) != NULL)
		strcpy(qname[i++], p);

	// print result
	i = 0;
	while(i++ <= ARGNUM - 1)
	{
		exist = 0;
		fseek(fp, 0, SEEK_SET);
		fscanf(fp, "%s\n", tmp);

		while(fscanf(fp, "%s\n", tmp) != EOF){
			id = strtok(tmp, ",");

			if(!strcmp(ids[i - 1], id)){
				exist = 1;
				j = 0;
				first = 0;
				while((p = strtok(NULL, ",")) != NULL){
					if(atof(p) == 0){
						if(!first){
							printf("%s's wrong answer :\n", id);
							first = 1;
						}
						if(strcmp(qname[j], "sum"))
							printf("%s    ", qname[j]);
					}
					j++;
				}
				printf("\n");
			}
		}

		if(!exist)
			printf("%s doesn't exist!\n", ids[i - 1]);
	}

	fclose(fp);
}

void do_mOption(char *ansDir)
{
	double newScore;
	char modiName[FILELEN];
	char filename[FILELEN];
	char *ptr;
	int i;

	ptr = malloc(sizeof(char) * FILELEN);

	while(1){

		printf("Input question's number to modify >> ");
		scanf("%s", modiName);

		if(strcmp(modiName, "no") == 0)
			break;

		for(i=0; i < sizeof(score_table) / sizeof(score_table[0]); i++){
			strcpy(ptr, score_table[i].qname);
			ptr = strtok(ptr, ".");
			if(!strcmp(ptr, modiName)){
				printf("Current score : %.2f\n", score_table[i].score);
				printf("New score : ");
				scanf("%lf", &newScore);
				getchar();
				score_table[i].score = newScore;
				break;
			}
		}
	}

	sprintf(filename, "./%s", "score_table.csv");
	write_scoreTable(filename);
	free(ptr);

}

int is_exist(char (*src)[FILELEN], char *target)
{
	int i = 0;

	while(1)
	{
		if(i >= ARGNUM)
			return false;
		else if(!strcmp(src[i], ""))
			return false;
		else if(!strcmp(src[i++], target))
			return true;
	}
	return false;
}

// main에서 stuDir, ansDir 경로 저장 후 처음 호출되는 함수
void set_scoreTable(char *ansDir)
{
	char filename[FILELEN];  // score_table.csv 경로 저장, 128 바이트

	sprintf(filename, "./%s", "score_table.csv");  // ssu_score이 있는 디렉토리의 score_table.csv 경로 저장

	if(access(filename, F_OK) == 0)  // score_table.csv 가 존재한다면
		read_scoreTable(filename);   // score_table.csv 읽기
	else{							 // score_table.csv 가 존재하지 않는다면
		make_scoreTable(ansDir);     // score_table.csv 생성
		write_scoreTable(filename);  // 생성한 테이블에 값 쓰기
	}
}

/* score_table.csv가 존재 시 호출 */
/* 문제와 점수를 읽어 구조체 배열  */
/* score_table에 저장한다		  */
/* path : score_table의 경로	 */
void read_scoreTable(char *path)
{
	FILE *fp;			  // score_table.csv를 가리키는 파일 포인터
	char qname[FILELEN];  // 임시로 문제의 이름을 담는 배열. 128 바이트
	char score[BUFLEN];   // 임시로 점수를 담는 배열. 1024 바이트
	int idx = 0;		  // 구조체 배열 score_table의 idx번째 요소 저장 시 사용

	if((fp = fopen(path, "r")) == NULL){  					// score_table.csv 파일을 읽기 모드로 열기
		fprintf(stderr, "file open error for %s\n", path);  // 열리지 않는다면 에러 처리 후 종료
		return ;
	}

	while(fscanf(fp, "%[^,],%s\n", qname, score) != EOF){  // ,를 기준으로 qname과 score 저장
		strcpy(score_table[idx].qname, qname);			   // 문제 이름 저장
		score_table[idx++].score = atof(score);			   // 점수 저장
	}

	fclose(fp);  // 파일 닫기
}

/* score_table.csv가 존재하지 않을 경우 호출 */
/* ansDir : 정답 디렉토리 경로 */
void make_scoreTable(char *ansDir)  // 정답 디렉토리로부터 읽어서 csv를 만드나?
{
	int type, num;  // type : 파일의 종류 저장(TEXTFILE(3) or CFILE(4) or -1), num : 문제 배점 방식(1 or 2)
	double score, bscore, pscore;  // bscore : 빈칸 문제 배점, pscore : 프로그램 문제 배점
	struct dirent *dirp, *c_dirp;
	DIR *dp, *c_dp;  // dp : 디렉토리를 가리키는 포인터
	char *tmp;
	int idx = 0;  // score_table에 정답 파일 이름 저장시 사용. 정답 파일 개수
	int i;

	num = get_create_type();  // 문제별 점수 설정 방식 고르기

	if(num == 1)  // 빈칸 채우기 문제와 프로그램 문제, 두 종류의 점수만 설정
	{
		printf("Input value of blank question : ");
		scanf("%lf", &bscore);  // 빈칸 문제 점수 설정
		printf("Input value of program question : ");
		scanf("%lf", &pscore);  // 프로그램 문제 점수 설정
	}

	if((dp = opendir(ansDir)) == NULL){  					 // 정답 디렉토리 열기
		fprintf(stderr, "open dir error for %s\n", ansDir);  // 열리지 않는다면 에러 처리 후 종료
		return;
	}

	// ansDir의 하위의 정답 파일 이름들을 score_table의 qname에 저장한다 
	while((dirp = readdir(dp)) != NULL){  // 현재 디렉토리의 하위에 파일이 있다면

		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))  // 파일 .와 ..은 건너뛰기
			continue;

		if((type = get_file_type(dirp->d_name)) < 0)   // 텍스트 파일(3) or .c 파일(4)이 아니면 건너뛰기
			continue;

		strcpy(score_table[idx].qname, dirp->d_name);  // 구조체 배열 score_table의 qname에 정답 파일 이름 저장

		idx++;  // 다음 칸 가리키기
	}

	closedir(dp);  // 파일 포인터 닫기
	sort_scoreTable(idx);

	for(i = 0; i < idx; i++)  // 정답 파일 수 만큼 반복
	{
		type = get_file_type(score_table[i].qname);

		if(num == 1)
		{
			if(type == TEXTFILE)
				score = bscore;
			else if(type == CFILE)
				score = pscore;
		}
		else if(num == 2)
		{
			printf("Input of %s: ", score_table[i].qname);
			scanf("%lf", &score);
		}

		score_table[i].score = score;
	}
}

void write_scoreTable(char *filename)
{
	int fd;
	char tmp[BUFLEN];
	int i;
	int num = sizeof(score_table) / sizeof(score_table[0]);

	if((fd = creat(filename, 0666)) < 0){
		fprintf(stderr, "creat error for %s\n", filename);
		return;
	}

	for(i = 0; i < num; i++)
	{
		if(score_table[i].score == 0)
			break;

		sprintf(tmp, "%s,%.2f\n", score_table[i].qname, score_table[i].score);
		write(fd, tmp, strlen(tmp));
	}

	close(fd);
}


void set_idTable(char *stuDir)
{
	struct stat statbuf;
	struct dirent *dirp;
	DIR *dp;
	char tmp[BUFLEN];
	int num = 0;

	if((dp = opendir(stuDir)) == NULL){
		fprintf(stderr, "opendir error for %s\n", stuDir);
		exit(1);
	}

	while((dirp = readdir(dp)) != NULL){
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		sprintf(tmp, "%s/%s", stuDir, dirp->d_name);
		stat(tmp, &statbuf);

		if(S_ISDIR(statbuf.st_mode))
			strcpy(id_table[num++], dirp->d_name);
		else
			continue;
	}
	closedir(dp);

	sort_idTable(num);
}

/* sort table by SCHOOL_ID */
/* size : num of students  */
void sort_idTable(int size)  
{
	int i, j;
	char tmp[10];  // save SCHOOL_ID temp

	// bubble sort by SCHOOL_ID ASC
	for(i = 0; i < size - 1; i++){
		for(j = 0; j < size - 1 -i; j++){
			if(strcmp(id_table[j], id_table[j+1]) > 0){
				strcpy(tmp, id_table[j]);
				strcpy(id_table[j], id_table[j+1]);
				strcpy(id_table[j+1], tmp);
			}
		}
	}
}

/* score_table을 파일명 기준으로 정렬한다    */
/* size : make_scoreTable()에서 찾은 */
/* ansDir 하위의 정답 파일 수 idx의 값 */
void sort_scoreTable(int size)  // 아마 1 10 2 3 이런걸 1 2 3 10 으로 정렬해주는 듯. 정답 파일 이름 구조랑 gdb로 get_qname_num 봐야 알겠다.
{
	int i, j;
	struct ssu_scoreTable tmp;
	int num1_1, num1_2;
	int num2_1, num2_2;

	for(i = 0; i < size - 1; i++){
		for(j = 0; j < size - 1 - i; j++){

			get_qname_number(score_table[j].qname, &num1_1, &num1_2);
			get_qname_number(score_table[j+1].qname, &num2_1, &num2_2);

			if((num1_1 > num2_1) || ((num1_1 == num2_1) && (num1_2 > num2_2))){

				memcpy(&tmp, &score_table[j], sizeof(score_table[0]));
				memcpy(&score_table[j], &score_table[j+1], sizeof(score_table[0]));
				memcpy(&score_table[j+1], &tmp, sizeof(score_table[0]));
			}
		}
	}
}

/* save qname's score in num1, num2*/
/* 정답 파일 명으로 num1과 num2에 값 저장 */
void get_qname_number(char *qname, int *num1, int *num2)
{
	char *p;
	char dup[FILELEN];  // 정답 파일 이름 담는 배열

	strncpy(dup, qname, strlen(qname));  // dup에 정답 파일 명 복사
	*num1 = atoi(strtok(dup, "-."));  // gdb로 자세히 보기 아마 1-1, 1-2에서 뒤 숫자를 분리하나보다

	p = strtok(NULL, "-.");
	if(p == NULL)
		*num2 = 0;
	else
		*num2 = atoi(p);
}

int get_create_type()
{
	int num;

	while(1)
	{
		printf("score_table.csv file doesn't exist in TREUDIR!\n");
		printf("1. input blank question and program question's score. ex) 0.5 1\n");
		printf("2. input all question's score. ex) Input value of 1-1: 0.1\n");
		printf("select type >> ");
		scanf("%d", &num);

		if(num != 1 && num != 2)
			printf("not correct number!\n");
		else
			break;
	}

	return num;
}

void score_students()
{
	double score = 0;
	int num;
	int fd;
	char tmp[BUFLEN];
	int size = sizeof(id_table) / sizeof(id_table[0]);

	if((fd = creat("score.csv", 0666)) < 0){
		fprintf(stderr, "creat error for score.csv");
		return;
	}
	write_first_row(fd);

	for(num = 0; num < size; num++)
	{
		if(!strcmp(id_table[num], ""))
			break;

		sprintf(tmp, "%s,", id_table[num]);
		write(fd, tmp, strlen(tmp)); 

		score += score_student(fd, id_table[num]);
	}

	printf("Total average : %.2f\n", score / num);

	close(fd);
}

double score_student(int fd, char *id)
{
	int type;
	double result;
	double score = 0;
	int i;
	char tmp[BUFLEN];
	int size = sizeof(score_table) / sizeof(score_table[0]);

	for(i = 0; i < size ; i++)
	{
		if(score_table[i].score == 0)
			break;

		sprintf(tmp, "%s/%s/%s", stuDir, id, score_table[i].qname);

		if(access(tmp, F_OK) < 0)
			result = false;
		else
		{
			if((type = get_file_type(score_table[i].qname)) < 0)
				continue;

			if(type == TEXTFILE)
				result = score_blank(id, score_table[i].qname);
			else if(type == CFILE)
				result = score_program(id, score_table[i].qname);
		}

		if(result == false)
			write(fd, "0,", 2);
		else{
			if(result == true){
				score += score_table[i].score;
				sprintf(tmp, "%.2f,", score_table[i].score);
			}
			else if(result < 0){
				score = score + score_table[i].score + result;
				sprintf(tmp, "%.2f,", score_table[i].score + result);
			}
			write(fd, tmp, strlen(tmp));
		}
	}

	printf("%s is finished. score : %.2f\n", id, score); 

	sprintf(tmp, "%.2f\n", score);
	write(fd, tmp, strlen(tmp));

	return score;
}

void write_first_row(int fd)
{
	int i;
	char tmp[BUFLEN];
	int size = sizeof(score_table) / sizeof(score_table[0]);

	write(fd, ",", 1);

	for(i = 0; i < size; i++){
		if(score_table[i].score == 0)
			break;

		sprintf(tmp, "%s,", score_table[i].qname);
		write(fd, tmp, strlen(tmp));
	}
	write(fd, "sum\n", 4);
}

char *get_answer(int fd, char *result)
{
	char c;
	int idx = 0;

	memset(result, 0, BUFLEN);
	while(read(fd, &c, 1) > 0)
	{
		if(c == ':')
			break;

		result[idx++] = c;
	}
	if(result[strlen(result) - 1] == '\n')
		result[strlen(result) - 1] = '\0';

	return result;
}

int score_blank(char *id, char *filename)
{
	char tokens[TOKEN_CNT][MINLEN];
	node *std_root = NULL, *ans_root = NULL;
	int idx, start;
	char tmp[BUFLEN];
	char s_answer[BUFLEN], a_answer[BUFLEN];
	char qname[FILELEN];
	int fd_std, fd_ans;
	int result = true;
	int has_semicolon = false;

	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));

	sprintf(tmp, "%s/%s/%s", stuDir, id, filename);
	fd_std = open(tmp, O_RDONLY);
	strcpy(s_answer, get_answer(fd_std, s_answer));

	if(!strcmp(s_answer, "")){
		close(fd_std);
		return false;
	}

	if(!check_brackets(s_answer)){
		close(fd_std);
		return false;
	}

	strcpy(s_answer, ltrim(rtrim(s_answer)));

	if(s_answer[strlen(s_answer) - 1] == ';'){
		has_semicolon = true;
		s_answer[strlen(s_answer) - 1] = '\0';
	}

	if(!make_tokens(s_answer, tokens)){
		close(fd_std);
		return false;
	}

	idx = 0;
	std_root = make_tree(std_root, tokens, &idx, 0);

	sprintf(tmp, "%s/%s", ansDir, filename);
	fd_ans = open(tmp, O_RDONLY);

	while(1)
	{
		ans_root = NULL;
		result = true;

		for(idx = 0; idx < TOKEN_CNT; idx++)
			memset(tokens[idx], 0, sizeof(tokens[idx]));

		strcpy(a_answer, get_answer(fd_ans, a_answer));

		if(!strcmp(a_answer, ""))
			break;

		strcpy(a_answer, ltrim(rtrim(a_answer)));

		if(has_semicolon == false){
			if(a_answer[strlen(a_answer) -1] == ';')
				continue;
		}

		else if(has_semicolon == true)
		{
			if(a_answer[strlen(a_answer) - 1] != ';')
				continue;
			else
				a_answer[strlen(a_answer) - 1] = '\0';
		}

		if(!make_tokens(a_answer, tokens))
			continue;

		idx = 0;
		ans_root = make_tree(ans_root, tokens, &idx, 0);

		compare_tree(std_root, ans_root, &result);

		if(result == true){
			close(fd_std);
			close(fd_ans);

			if(std_root != NULL)
				free_node(std_root);
			if(ans_root != NULL)
				free_node(ans_root);
			return true;

		}
	}

	close(fd_std);
	close(fd_ans);

	if(std_root != NULL)
		free_node(std_root);
	if(ans_root != NULL)
		free_node(ans_root);

	return false;
}

double score_program(char *id, char *filename)
{
	double compile;
	int result;

	compile = compile_program(id, filename);

	if(compile == ERROR || compile == false)
		return false;

	result = execute_program(id, filename);

	if(!result)
		return false;

	if(compile < 0)
		return compile;

	return true;
}

int is_thread(char *qname)
{
	int i;
	int size = sizeof(threadFiles) / sizeof(threadFiles[0]);

	for(i = 0; i < size; i++){
		if(!strcmp(threadFiles[i], qname))
			return true;
	}
	return false;
}

double compile_program(char *id, char *filename)
{
	int fd;
	char tmp_f[BUFLEN], tmp_e[BUFLEN];
	char command[BUFLEN];
	char qname[FILELEN];
	int isthread;
	off_t size;
	double result;

	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));

	isthread = is_thread(qname);

	sprintf(tmp_f, "%s/%s", ansDir, filename);
	sprintf(tmp_e, "%s/%s.exe", ansDir, qname);

	if(tOption && isthread)
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f);
	else
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f);

	sprintf(tmp_e, "%s/%s_error.txt", ansDir, qname);
	fd = creat(tmp_e, 0666);

	redirection(command, fd, STDERR);
	size = lseek(fd, 0, SEEK_END);
	close(fd);
	unlink(tmp_e);

	if(size > 0)
		return false;

	sprintf(tmp_f, "%s/%s/%s", stuDir, id, filename);
	sprintf(tmp_e, "%s/%s/%s.stdexe", stuDir, id, qname);

	if(tOption && isthread)
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f);
	else
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f);

	sprintf(tmp_f, "%s/%s/%s_error.txt", stuDir, id, qname);
	fd = creat(tmp_f, 0666);

	redirection(command, fd, STDERR);
	size = lseek(fd, 0, SEEK_END);
	close(fd);

	if(size > 0){
		if(eOption)
		{
			sprintf(tmp_e, "%s/%s", errorDir, id);
			if(access(tmp_e, F_OK) < 0)
				mkdir(tmp_e, 0755);

			sprintf(tmp_e, "%s/%s/%s_error.txt", errorDir, id, qname);
			rename(tmp_f, tmp_e);

			result = check_error_warning(tmp_e);
		}
		else{ 
			result = check_error_warning(tmp_f);
			unlink(tmp_f);
		}

		return result;
	}

	unlink(tmp_f);
	return true;
}

double check_error_warning(char *filename)
{
	FILE *fp;
	char tmp[BUFLEN];
	double warning = 0;

	if((fp = fopen(filename, "r")) == NULL){
		fprintf(stderr, "fopen error for %s\n", filename);
		return false;
	}

	while(fscanf(fp, "%s", tmp) > 0){
		if(!strcmp(tmp, "error:"))
			return ERROR;
		else if(!strcmp(tmp, "warning:"))
			warning += WARNING;
	}

	return warning;
}

int execute_program(char *id, char *filename)
{
	char std_fname[BUFLEN], ans_fname[BUFLEN];
	char tmp[BUFLEN];
	char qname[FILELEN];
	time_t start, end;
	pid_t pid;
	int fd;

	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));

	sprintf(ans_fname, "%s/%s.stdout", ansDir, qname);
	fd = creat(ans_fname, 0666);

	sprintf(tmp, "%s/%s.exe", ansDir, qname);
	redirection(tmp, fd, STDOUT);
	close(fd);

	sprintf(std_fname, "%s/%s/%s.stdout", stuDir, id, qname);
	fd = creat(std_fname, 0666);

	sprintf(tmp, "%s/%s/%s.stdexe &", stuDir, id, qname);

	start = time(NULL);
	redirection(tmp, fd, STDOUT);

	sprintf(tmp, "%s.stdexe", qname);
	while((pid = inBackground(tmp)) > 0){
		end = time(NULL);

		if(difftime(end, start) > OVER){
			kill(pid, SIGKILL);
			close(fd);
			return false;
		}
	}

	close(fd);

	return compare_resultfile(std_fname, ans_fname);
}

pid_t inBackground(char *name)
{
	pid_t pid;
	char command[64];
	char tmp[64];
	int fd;
	off_t size;

	memset(tmp, 0, sizeof(tmp));
	fd = open("background.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);

	sprintf(command, "ps | grep %s", name);
	redirection(command, fd, STDOUT);

	lseek(fd, 0, SEEK_SET);
	read(fd, tmp, sizeof(tmp));

	if(!strcmp(tmp, "")){
		unlink("background.txt");
		close(fd);
		return 0;
	}

	pid = atoi(strtok(tmp, " "));
	close(fd);

	unlink("background.txt");
	return pid;
}

int compare_resultfile(char *file1, char *file2)
{
	int fd1, fd2;
	char c1, c2;
	int len1, len2;

	fd1 = open(file1, O_RDONLY);
	fd2 = open(file2, O_RDONLY);

	while(1)
	{
		while((len1 = read(fd1, &c1, 1)) > 0){
			if(c1 == ' ') 
				continue;
			else 
				break;
		}
		while((len2 = read(fd2, &c2, 1)) > 0){
			if(c2 == ' ') 
				continue;
			else 
				break;
		}

		if(len1 == 0 && len2 == 0)
			break;

		to_lower_case(&c1);
		to_lower_case(&c2);

		if(c1 != c2){
			close(fd1);
			close(fd2);
			return false;
		}
	}
	close(fd1);
	close(fd2);
	return true;
}

void redirection(char *command, int new, int old)
{
	int saved;

	saved = dup(old);
	dup2(new, old);

	system(command);

	dup2(saved, old);
	close(saved);
}

// 파일의 타입을 리턴하는 함수 
int get_file_type(char *filename)
{
	char *extension = strrchr(filename, '.');  // .포함 확장자를 저장

	if(!strcmp(extension, ".txt"))		// 확장자가 .txt면
		return TEXTFILE;  				// 텍스트 파일(3) 리턴
	else if (!strcmp(extension, ".c"))  // 확장자가 .c이면
		return CFILE;  					// .c 파일(4) 리턴
	else  								// 텍스트 파일이나 .c 파일이 아니면
		return -1;  					// -1 리턴
}

void rmdirs(const char *path)
{
	struct dirent *dirp;
	struct stat statbuf;
	DIR *dp;
	//char tmp[50]; 
	char tmp[4096];  // path max length = 4096

	if((dp = opendir(path)) == NULL)
		return;

	while((dirp = readdir(dp)) != NULL)
	{
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;

		sprintf(tmp, "%s/%s", path, dirp->d_name);

		if(lstat(tmp, &statbuf) == -1)
			continue;

		if(S_ISDIR(statbuf.st_mode))
			rmdirs(tmp);
		else
			unlink(tmp);
	}

	closedir(dp);
	rmdir(path);
}

void to_lower_case(char *c)
{
	if(*c >= 'A' && *c <= 'Z')
		*c = *c + 32;
}

void print_usage()
{
	printf("Usage : ssu_score <STUDENTDIR> <TRUEDIR> [OPTION]\n");
	printf("Option : \n");
	printf(" -m                modify question's score\n");
	printf(" -e <DIRNAME>      print error on 'DIRNAME/ID/qname_error.txt' file \n");
	printf(" -t <QNAMES>       compile QNAME.C with -lpthread option\n");
	printf(" -t <IDS>          print ID's wrong questions\n");
	printf(" -h                print usage\n");
}
