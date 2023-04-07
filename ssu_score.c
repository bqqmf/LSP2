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
extern char id_table[SNUM][10];                  // 다른 파일에서 id_table 접근 가능

struct ssu_scoreTable score_table[QNUM];         // max 100개의 question 저장
char id_table[SNUM][10];                         // max save 100 students's STUDENT_ID

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

	// -i option 없이 ./ssu_score student_dir answer_dir 실행 시
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
	// stuDir에 학생 디렉토리의 절대 경로 저장
	getcwd(stuDir, BUFLEN);

	// ./ssu_score를 실행한 디렉토리로 다시 돌아오기
	chdir(saved_path);

	// 정답 디렉토리로 디렉토리 변경
	if(chdir(ansDir) < 0){
		fprintf(stderr, "%s doesn't exist\n", ansDir);  // ansDir이 존재하지 않는다면 에러 출력 후 종료
		return;
	}
	// ansDir에 정답 디렉토리의 절대 경로 저장
	getcwd(ansDir, BUFLEN);

	// ./ssu_score를 실행한 디렉토리로 다시 돌아오기
	chdir(saved_path);

	set_scoreTable(ansDir);  // score_table.csv 내용을 구조체 배열 score_table에 저장
	set_idTable(stuDir);	 // STD_DIR 하위의 학번 폴더들로 id_table 생성

	// -m 입력 시 수행
	if(mOption)
		do_mOption();

	printf("grading student's test papers..\n");
	score_students();  // 학생들 점수 매기고 score.csv 생성

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
		make_scoreTable(ansDir);     // fill score_table[] from ANS_DIR
		write_scoreTable(filename);  // create score_table.csv 
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
/* fill score_table[] and sort from ANS_DIR */
/* ansDir : 정답 디렉토리 경로 */
void make_scoreTable(char *ansDir)  // 정답 디렉토리로부터 읽어서 csv를 만드나?
{
	int type, num;  // type : 파일의 종류 저장(TEXTFILE(3) or CFILE(4) or -1), num : 문제 배점 방식(1 or 2)
	double score, bscore, pscore;  // score : save baejum by qname,  bscore : 빈칸 문제 배점, pscore : 프로그램 문제 배점
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
	sort_scoreTable(idx);  // sort scoreTable by qname ASC

	// score_table에 qname의 배점 저장
	for(i = 0; i < idx; i++)  // 정답 파일 수 만큼 반복
	{
		type = get_file_type(score_table[i].qname);  // qname 파일의 유형 가져오기. TEXTFILE(3) or CFILE(4) or -1

		if(num == 1)  // b, pscore에 배점이 저장되어있다.
		{
			if(type == TEXTFILE)    // .txt 파일 ex) 1-1.txt
				score = bscore;     // 배점 저장
			else if(type == CFILE)  // .c 파일 ex) 20.c
				score = pscore;     // 배점 저장
		}
		else if(num == 2)  // 문제마다 배점이 다른 경우
		{
			printf("Input of %s: ", score_table[i].qname);  // 문제 이름 출력
			scanf("%lf", &score);  // 문제의 배점 입력
		}

		score_table[i].score = score;  // i번째 문제의 배점 저장
	}
}

/* score_table 이 채워진 후 호출됨 */
/* score_table의 값으로 score_table.csv 생성 */
/* filename : score_table.csv의 절대 경로 */
void write_scoreTable(char *filename)
{
	int fd;  // score_table.csv의 fd
	char tmp[BUFLEN];
	int i;
	int num = sizeof(score_table) / sizeof(score_table[0]);  // 정답 파일 수

	if((fd = creat(filename, 0666)) < 0){  // score_table.csv 생성
		fprintf(stderr, "creat error for %s\n", filename);  // creat 예외 처리 후 종료
		return;
	}

	for(i = 0; i < num; i++)  // n번 반복
	{
		if(score_table[i].score == 0)  // score_table의 내용을 다 썼다면
			break;

		// save 문제 이름과 배점 저장
		sprintf(tmp, "%s,%.2f\n", score_table[i].qname, score_table[i].score);
		write(fd, tmp, strlen(tmp));  // score_table.csv에 쓰기
	}

	close(fd);
}

/* set_scoreTable() 이후 호출 */
/* STD_DIR 하위 폴더의 학번들로 id_table 채우기 */
/* stuDir : STD_DIR의 절대 경로 */
void set_idTable(char *stuDir)
{
	struct stat statbuf;  // 파일 정보 저장
	struct dirent *dirp;  // dir 포인터
	DIR *dp;              // STD_DIR 용 포인터
	char tmp[BUFLEN];     // 1024 바이트
	int num = 0;          // STD_DIR의 학생 수

	if((dp = opendir(stuDir)) == NULL){  // STD_DIR 디렉토리 열기
		fprintf(stderr, "opendir error for %s\n", stuDir);  // opendir 예외처리 후 종료
		exit(1);
	}

	while((dirp = readdir(dp)) != NULL){  // 하위 파일이 존재하는 동안
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))  // . .. 패스
			continue;

		// 하위 파일의 절대 경로를 tmp에 저장
		sprintf(tmp, "%s/%s", stuDir, dirp->d_name);  
		// 파일 정보 저장
		stat(tmp, &statbuf);

		if(S_ISDIR(statbuf.st_mode))  // 파일이 디렉토리라면
			strcpy(id_table[num++], dirp->d_name);  // id_table에 학번 저장
		else
			continue;  // 일반 파일은 패스
	}
	closedir(dp);  // 디렉토리 닫기

	sort_idTable(num);  // 학번으로 id_table 정렬
}

/* 학번 기준으로 id_table 정렬 */
/* size : 학생 수 */
void sort_idTable(int size)  
{
	int i, j;  // 정렬에 쓰이는 인덱스
	char tmp[10];  // 학번 임시 저장

	// 오름차순으로 학번 기준 정렬
	for(i = 0; i < size - 1; i++){
		for(j = 0; j < size - 1 -i; j++){
			if(strcmp(id_table[j], id_table[j+1]) > 0){  // j의 학번이 더 높다면
				strcpy(tmp, id_table[j]);  // j의 학번을 tmp에 저장
				strcpy(id_table[j], id_table[j+1]);  // j+1의 학번을 j에 저장
				strcpy(id_table[j+1], tmp);  // tmp의 학번을 j+1에 저장
			}
		}
	}
}

/* score_table을 파일명 기준으로 정렬한다 */
/* size : make_scoreTable()에서 찾은 */
/* ansDir 하위의 정답 파일 수 */
void sort_scoreTable(int size)  // 1-1.txt, ..., 29.c
{
	int i, j;  // index for sort
	struct ssu_scoreTable tmp;  // tmp for swap
	int num1_1, num1_2;  // a-b.txt -> _1 : a, _2 : b
	int num2_1, num2_2;  // _1 : main qname num, _2 : sub qname num

	// 문제 번호로 score_table 오름차순 정렬
	for(i = 0; i < size - 1; i++){
		for(j = 0; j < size - 1 - i; j++){  
			// j번째 문제 번호 가져오기 
			get_qname_number(score_table[j].qname, &num1_1, &num1_2);  
			// j+1번째 문제 번호 가져오기 
			get_qname_number(score_table[j+1].qname, &num2_1, &num2_2);

			// j번째 문제 번호가 더 크다면 스왑
			if((num1_1 > num2_1) || ((num1_1 == num2_1) && (num1_2 > num2_2))){

				memcpy(&tmp, &score_table[j], sizeof(score_table[0]));  // tmp에 j번째 요소 저장
				memcpy(&score_table[j], &score_table[j+1], sizeof(score_table[0]));// j에 j+1 번째 요소 저장
				memcpy(&score_table[j+1], &tmp, sizeof(score_table[0]));  // j+1에 tmp 요소 저장
			}
		}
	}
}

/* num1과 num2에 qname의 문제 번호 저장 */
/* ex) 1-2.txt -> num1: 1, num2: 2 */
void get_qname_number(char *qname, int *num1, int *num2)
{
	char *p;  // 문제 번호 포인터
	char dup[FILELEN];  // 정답 파일 이름 담는 배열

	strncpy(dup, qname, strlen(qname));  // dup에 정답 파일 명 복사
	*num1 = atoi(strtok(dup, "-."));  // 문제 이름을 (- | .)로 분할하고 num1에 저장

	p = strtok(NULL, "-.");  // 한번 더 분할
	if(p == NULL)            // 더이상 -.가 없다면
		*num2 = 0;           // sub num is 0
	else                     // -.가 있다면
		*num2 = atoi(p);     // save sub num
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

/* id_table 세팅 후 호출됨 */
/* score.csv 생성 */
/* 학생들이 제출한 파일의 점수 매기기 */
void score_students()
{
	double score = 0;  // 학생들의 점수 저장
	int num; // 반복문을 위한 인덱스
	int fd;  // score.csv의 fd
	char tmp[BUFLEN];
	int size = sizeof(id_table) / sizeof(id_table[0]);  // id_table 크기

	if((fd = creat("score.csv", 0666)) < 0){  // score.csv 생성
		fprintf(stderr, "creat error for score.csv");  // creat 예외 처리
		return;
	}
	write_first_row(fd);  // score.csv에 첫번째 행 추가. 문제 번호들과 합계가 적힘

	for(num = 0; num < size; num++)  // 제출한 학생 수 만큼 반복
	{
		if(!strcmp(id_table[num], ""))  // 모든 학생을 읽었다면 종료
			break;

		sprintf(tmp, "%s,", id_table[num]);  // 학번에 , 붙이기
		write(fd, tmp, strlen(tmp));  // score.csv에 "학번," 쓰기

		score += score_student(fd, id_table[num]);  // 학생의 점수를 계산하여 score.csv에 쓰고 점수 누적
	}

	printf("Total average : %.2f\n", score / num);  // 학생들이 평균 출력

	close(fd);  // score.csv 닫기
}

/* score_students() 내부에서 호출 */
/* id가 제출한 정답을 채점하고 score.csv에 쓴다 */
/* fd : score.csv, id : 채점 할 학생의 학번 */
/* return : id의 정답의 총합 */
double score_student(int fd, char *id)
{
	int type;  // 문제의 유형 
	double result;  // 학생의 답안 제출 여부
	double score = 0;  // 점수
	int i;  // 반복분의 인덱스
	char tmp[BUFLEN];  // 학생이 제출한 답안의 절대 경로 또는 점수 저장
	int size = sizeof(score_table) / sizeof(score_table[0]);  // 답안 제출 학생 수

	for(i = 0; i < size ; i++)  // 학생 수 만큼 반복
	{
		if(score_table[i].score == 0)  // 모든 학생 채점이 끝나면 종료
			break;

		// 학번이 id인 학생의 제출 답안의 절대 경로 ex) stuDir/20230000/1-1.txt
		if (snprintf(tmp, sizeof(tmp), "%s/%s/%s", stuDir, id,
					score_table[i].qname) >= sizeof(tmp))
			// 경로 길이가 tmp보다 크다면 예외처리
			fprintf(stderr, "tag buffer overflow - string is truncated\n");

		if(access(tmp, F_OK) < 0)  // 학생이 제출한 답안이 존재하지 않으면
			result = false;  // 0 저장
		else  // 답안을 제출 했으면
		{
			if((type = get_file_type(score_table[i].qname)) < 0)  // 제출한 파일이 .txt나 .c가 아닌 경우 패스
				continue;

			if(type == TEXTFILE)  // 문제가 .txt 파일
				result = score_blank(id, score_table[i].qname);  // .txt 문제 채점
			else if(type == CFILE)  // 문제가 .c 파일
				result = score_program(id, score_table[i].qname); // .c 문제 채점
		}

		if(result == false)  // 학생이 i번째 문제의 답안을 제출하지 않았다면
			write(fd, "0,", 2);  // 0점 처리
		else{  // 제출 했다면
			if(result == true){  
				score += score_table[i].score;  // += graded score
				sprintf(tmp, "%.2f,", score_table[i].score);  // save score to tmp
			}
			else if(result < 0){
				score = score + score_table[i].score + result;
				sprintf(tmp, "%.2f,", score_table[i].score + result);
			}
			write(fd, tmp, strlen(tmp));
		}
	}

	printf("%s is finished. score : %.2f\n", id, score); 

	sprintf(tmp, "%.2f\n", score);  // tmp에 문자열로 총점 저장
	write(fd, tmp, strlen(tmp));  // 총점 score.csv에 쓰기

	return score;  // id 학생의 총점 리턴
}

/* called in score_students */
/* write attributes in score.csv */ 
/* ,1-1.txt,1-2.txt,...,29.c,sum */
/* fd : score.csv */
void write_first_row(int fd)
{
	int i;
	char tmp[BUFLEN];
	int size = sizeof(score_table) / sizeof(score_table[0]);  // questions num

	write(fd, ",", 1);  // write ,

	for(i = 0; i < size; i++){
		if(score_table[i].score == 0)  // if write ended
			break;

		sprintf(tmp, "%s,", score_table[i].qname);  // attatch , to qname
		write(fd, tmp, strlen(tmp));  // write to score.csv
	}
	write(fd, "sum\n", 4);  // lastly, write sum
}

/* called in score_blank() */
/* fd : fd for student's file, result : save result */
/* return : student's answer */
char *get_answer(int fd, char *result)
{
	char c;  // save byte
	int idx = 0;  // index for result

	memset(result, 0, BUFLEN);  // init result 0
	while(read(fd, &c, 1) > 0)  // read answer by 1 byte
	{
		if(c == ':')  // if char is :, end.
			break;

		result[idx++] = c;  // write answer by 1 byte
	}
	if(result[strlen(result) - 1] == '\n')  // replace \n to \0
		result[strlen(result) - 1] = '\0';

	return result;  // return answer from file
}

/* fill in the blank question */
/* id : hakbun, filename : file to grade ex) 1-1.txt */
int score_blank(char *id, char *filename)
{
	char tokens[TOKEN_CNT][MINLEN];  // 50 tokens, max length : 64
	node *std_root = NULL, *ans_root = NULL;  // root of student, answer tree
	int idx, start;
	char tmp[BUFLEN];
	char s_answer[BUFLEN], a_answer[BUFLEN];  // answer of student, answer
	char qname[FILELEN];
	int fd_std, fd_ans;  // fd for student, answer
	int result = true;
	int has_semicolon = false;

	memset(qname, 0, sizeof(qname));  // init 0
	// get qname without extension. ex) 1-1.txt -> 1.1
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));
	// abs path of student's file
	if (snprintf(tmp, sizeof(tmp), "%s/%s/%s", stuDir, id, filename) >= sizeof(tmp))
		fprintf(stderr, "tag buffer overflow - string is truncated\n");
	fd_std = open(tmp, O_RDONLY);  // open ans file
	strcpy(s_answer, get_answer(fd_std, s_answer));  // save student's answer

	if(!strcmp(s_answer, "")){  // student's answer is blank 
		close(fd_std); // close file
		return false; 
	}

	if(!check_brackets(s_answer)){  // if s_answer is grammarly wrong
		close(fd_std);  // close fd 
		return false;
	}

	strcpy(s_answer, ltrim(rtrim(s_answer)));  // remove left and right spaces

	if(s_answer[strlen(s_answer) - 1] == ';'){  // answer ends with ;
		has_semicolon = true;  // ; flag on
		s_answer[strlen(s_answer) - 1] = '\0';  // replace ; to \0
	}

	if(!make_tokens(s_answer, tokens)){  // tokenize s_answer and save to tokens
		close(fd_std);
		return false;
	}

	idx = 0;
	// make student's tree with tokens
	std_root = make_tree(std_root, tokens, &idx, 0); 

	if (snprintf(tmp, sizeof(tmp), "%s/%s", ansDir, filename) >= sizeof(tmp))
		fprintf(stderr, "tag buffer overflow - string is truncated\n");
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

/* program question */
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

	if (snprintf(tmp_f, sizeof(tmp_f), "%s/%s", ansDir, filename) >= sizeof(tmp_f))
		fprintf(stderr, "tag buffer overflow - string is truncated\n");
	if (snprintf(tmp_e, sizeof(tmp_e), "%s/%s.exe", ansDir, qname) >= sizeof(tmp_e))
		fprintf(stderr, "tag buffer overflow - string is truncated\n");

	if(tOption && isthread)
		if (snprintf(command, sizeof(command), "gcc -o %s %s -lpthread", tmp_e, tmp_f) >= sizeof(command))

			fprintf(stderr, "tag buffer overflow - string is truncated\n");
		else
			if (snprintf(command, sizeof(command), "gcc -o %s %s", tmp_e, tmp_f) >= sizeof(command))
				fprintf(stderr, "tag buffer overflow - string is truncated\n");

	if (snprintf(tmp_e, sizeof(tmp_e), "%s/%s_error.txt", ansDir, qname) >= sizeof(tmp_e))
		fprintf(stderr, "tag buffer overflow - string is truncated\n");
	fd = creat(tmp_e, 0666);

	redirection(command, fd, STDERR);
	size = lseek(fd, 0, SEEK_END);
	close(fd);
	unlink(tmp_e);

	if(size > 0)
		return false;

	if (snprintf(tmp_f, sizeof(tmp_f), "%s/%s/%s", stuDir, id, filename) >= sizeof(tmp_f))
		fprintf(stderr, "tag buffer overflow - string is truncated\n");
	if (snprintf(tmp_e, sizeof(tmp_e), "%s/%s/%s.stdexe", stuDir, id, qname) >= sizeof(tmp_e))
		fprintf(stderr, "tag buffer overflow - string is truncated\n");

	if(tOption && isthread)
		if (snprintf(command, sizeof(command), "gcc -o %s %s -lpthread", tmp_e, tmp_f) >= sizeof(command))
			fprintf(stderr, "tag buffer overflow - string is truncated\n");
		else
			if (snprintf(command, sizeof(command), "gcc -o %s %s", tmp_e, tmp_f) >= sizeof(command))
				fprintf(stderr, "tag buffer overflow - string is truncated\n");

	if (snprintf(tmp_f, sizeof(tmp_f), "%s/%s/%s_error.txt", stuDir, id, qname) >= sizeof(tmp_f))
		fprintf(stderr, "tag buffer overflow - string is truncated\n");
	fd = creat(tmp_f, 0666);

	redirection(command, fd, STDERR);
	size = lseek(fd, 0, SEEK_END);
	close(fd);

	if(size > 0){
		if(eOption)
		{
			if (snprintf(tmp_e, sizeof(tmp_e), "%s/%s", errorDir, id) >= sizeof(tmp_e))
				fprintf(stderr, "tag buffer overflow - string is truncated\n");
			if(access(tmp_e, F_OK) < 0)
				mkdir(tmp_e, 0755);

			if (snprintf(tmp_e, sizeof(tmp_e), "%s/%s/%s_error.txt", errorDir, id, qname) >= sizeof(tmp_e))
				fprintf(stderr, "tag buffer overflow - string is truncated\n");
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

	if (snprintf(ans_fname, sizeof(ans_fname), "%s/%s.stdout", ansDir, qname) >= sizeof(ans_fname))
		fprintf(stderr, "tag buffer overflow - string is truncated\n");
	fd = creat(ans_fname, 0666);

	if (snprintf(tmp, sizeof(tmp), "%s/%s.exe", ansDir, qname) >= sizeof(tmp))
		fprintf(stderr, "tag buffer overflow - string is truncated\n");
	redirection(tmp, fd, STDOUT);
	close(fd);

	if (snprintf(std_fname, sizeof(std_fname), "%s/%s/%s.stdout", stuDir, id, qname) >= sizeof(std_fname))
		fprintf(stderr, "tag buffer overflow - string is truncated\n");
	fd = creat(std_fname, 0666);

	if (snprintf(tmp, sizeof(tmp), "%s/%s/%s.stdexe &", stuDir, id, qname) >= sizeof(tmp)) 
		fprintf(stderr, "tag buffer overflow - string is truncated\n");

	start = time(NULL);
	redirection(tmp, fd, STDOUT);

	if (snprintf(tmp, sizeof(tmp), "%s.stdexe", qname) >= sizeof(tmp))
		fprintf(stderr, "tag buffer overflow - string is truncated\n");
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

/* should i replace system(command) ? */
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
