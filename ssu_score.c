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
char currentDir[BUFLEN]; // cwd 경로. compile_program에서 사용
char ANS_Dir[BUFLEN];  // ./ANS 경로. compile_program에서 사용
char STD_Dir[BUFLEN];  // ./STD 경로. compile_program에서 사용
char score_csv_path[BUFLEN];  // ./ANS/score.csv
char score_table_path[BUFLEN];  // score_table.csv path
char threadFiles[ARGNUM][FILELEN];  // 5개까지 -lpthread option possible 
char c_opt_students[ARGNUM][10];  // using in c option. print with score
char iIDs[SNUM][10];  // using in i option
char category[10] = {0};  // using in s option
int is_ASC = 0;  // using in s option

ID_node *pHEAD;  // using in p option. head of Q_nodes
ID_node *pREAR;  // using in p option. rear of Q_nodes
				 
sorted_node *sHEAD;  // using in s option.
sorted_node *sREAR;  // using in s option.

int eOption = false;
int tOption = false;
int hOption = false;
int mOption = false;
int pOption = false;
int nOption = false;
int cOption = false;
int sOption = false;

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

	create_ANS_STD_Dir();  // ./ANS, ./STD 생성

	getcwd(saved_path, BUFLEN);  // saved_path에 현재 작업 디렉토리 저장

	if (snprintf(score_table_path, sizeof(score_table_path), "%s/score_table.csv", ANS_Dir) > sizeof(score_table_path))  // ./ANS/score_table.csv
		fprintf(stderr, "buffer overflow - string is truncated\n");

	if (snprintf(score_csv_path, sizeof(score_csv_path), "%s/score.csv", ANS_Dir) > sizeof(score_csv_path))  // ./ANS/score.csv
		fprintf(stderr, "buffer overflow - string is truncated\n");

	// -i option 없이 ./ssu_score student_dir answer_dir 실행 시
	if(argc >= 3 && strcmp(argv[1], "-i") != 0){  
		strcpy(stuDir, argv[1]);  // 학생 디렉토리 상대 경로 저장
		strcpy(ansDir, argv[2]);  // 정답 디렉토리 상대 경로 저장
	}

	// option check
	if(!check_option(argc, argv))
		exit(1);

	// if c_opt_students has element and iIDs
	if (strlen(c_opt_students[0]) > 0 && strlen(iIDs[0]) > 0) {
		fprintf(stderr, "use [STUDENTIDS ...] only one time\n");
		exit(1);
	}

	// h option
	if (hOption) {
		print_usage();
		return;
	}


	// m, e, t 옵션 말고 i 옵션만 사용하고 학생, 정답 디렉토리가 입력된 경우
	/*
	   if(!mOption && !eOption && !tOption && iOption 
	   && !strcmp(stuDir, "") && !strcmp(ansDir, "")){
	   do_pOption(iIDs);  // i 옵션 실행하고 종료
	   return;
	   }
	 */


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

	// print score_csv's path
	printf("result saved.. (%s)\n", score_csv_path);
	if (eOption)  // print errorDir path
		do_eOption();

	if (sOption)
		do_sOption();

	return;
}

int check_option(int argc, char *argv[])
{
	int i, j, k;
	int c;
	int exist = 0;
	// make <STD_DIR>'s sub ID_list
	// using scandir, ans_path
	char STD_ID_list[SNUM][10];  // save STUDENT_IDs in STD_DIR
	int stu_num = getStudentIDs(STD_ID_list);  // save students num

	while((c = getopt(argc, argv, "e:thmpn:cs1")) != -1)
	{
		switch(c){
			case 'e':  // make errorDir
				eOption = true;
				strcpy(errorDir, to_abs_path(optarg));  // save-e errorDir. let's make error dir to abs path 

				if(access(errorDir, F_OK) < 0)  // errorDir not exists, mkdir
					mkdir(errorDir, 0755);
				else{
					rmdirs(errorDir);  // already exists, rmdir
					mkdir(errorDir, 0755);  // mkdir
				}
				break;
			case 't':  // add -lpthread option
				tOption = true;
				i = optind;
				j = 0;  // index of threadFiles

				while(i < argc && argv[i][0] != '-'){

					if(j >= ARGNUM)  // catch exception
						printf("Maximum Number of Argument Exceeded.  :: %s\n", argv[i]);
					else{
						strcpy(threadFiles[j], argv[i]);  // add QNAME to threadFiles
					}
					i++; 
					j++;
				}
				break;
			case 'h':
				hOption = true;
				break;
			case 'm':
				mOption = true;
				break;

			case 'p':
				pOption = true;
				i = optind;
				j = 0;

				memset(iIDs[0], 0, 10);  // init iIDs[0] to 0

				while(i < argc && argv[i][0] != '-'){
					if(j >= ARGNUM)  // max 5 args
						printf("Maximum Number of Argument Exceeded. :: %s\n", argv[i]);
					else {
						int exist = 0;
						for (int k=0; k<stu_num; k++) {
							if (!strcmp(STD_ID_list[k], argv[i])) {  // check either STUDENT_ID in STD_DIR
								exist = 1;
								break;
							}
						}
						if (exist)
							strcpy(iIDs[j], argv[i]);  // save STDUENT_ID to iIDs;
						else {
							fprintf(stderr, "%s doesn't exist in %s\n", argv[i], stuDir);  // if STUDENT_ID not in STD_DIR
							exit(1);  // catch error
						}
					}
					i++;
					j++;
				}

				break;
			case 'n':
				nOption = true;
				strcpy(score_csv_path, to_abs_path(optarg));  // save abs path new score_table.csv
				char *extension = strrchr(optarg, '.');  // check .csv
				if (strcmp(extension, ".csv")) {
					fprintf(stderr, "not .csv file\n");
					exit(1);
				}

				char tmpdir[BUFLEN];
				strcpy(tmpdir, score_csv_path);
				tmpdir[strlen(tmpdir) - strlen(strrchr(tmpdir, '/'))] = 0;

				char mkpath[BUFLEN];
				strcat(mkpath, "/");
				memset(mkpath, 0, BUFLEN);
				char *dir;
				dir = strtok(tmpdir, "/");

				while (dir != NULL) {
				strcat(mkpath, "/");
					strcat(mkpath, dir);
					mkdir(mkpath, 0755);
					dir = strtok(NULL, "/");
				}
				
				if (access(score_csv_path, F_OK) == 0)  // if new score_table.csv exists,
					unlink(score_csv_path);  // remove old csv
				break;
			case 'c':
				cOption = true;
				i = optind;
				j = 0;  // index of c_opt_students 

				while(i < argc && argv[i][0] != '-'){

					if(j >= ARGNUM)  // catch exception
						printf("Maximum Number of Argument Exceeded.  :: %s\n", argv[i]);
					else{
						// check if argv[i] in ID_list
						int exists = 0;
						for (int k=0; k<stu_num; k++) {
							if (!strcmp(STD_ID_list[k], argv[i])) {  // check either STUDENT_ID in STD_DIR
								exists = 1;
								break;
							}
						}

						if (exists)
							strcpy(c_opt_students[j], argv[i]);  // add STD_ID to c_opt_students 
						else {
							fprintf(stderr, "%s doesn't exist in %s\n", argv[i], stuDir);  // if STUDENT_ID not in STD_DIR
							exit(1);  // catch error
						}
					}
					i++; 
					j++;
				}

				break;
			case 's':
				sOption = true;
				i = optind;

				if (argv[i] == 0 || argv[i+1] == 0) {
					fprintf(stderr, "usage : -s <CATEGORY> <1|-1>\n");
					exit(1);
				}

				if (strcmp(argv[i], "stdid") != 0 && strcmp(argv[i], "score") != 0) {
					fprintf(stderr, "usage : -s <CATEGORY> <1|-1>\n");
					exit(1);
				}

				if (strcmp(argv[i+1], "1") != 0 && strcmp(argv[i+1], "-1") != 0) {
					fprintf(stderr, "usage : -s <CATEGORY> <1|-1>\n");
					exit(1);
				}

				strcpy(category, argv[i]);

				if (!strcmp(argv[i+1], "1")) is_ASC = 1;
				else if (!strcmp(argv[i+1], "-1")) is_ASC = -1;

				printf("%s, %d\n", category, is_ASC);

				break;
			case '?':
				printf("Unkown option %c\n", optopt);
				return false;
		}
	}

	return true;
}

void do_mOption(char *ansDir)
{
	double newScore;  // save new baejum
	char modiName[FILELEN];  // qname to change baejum
	char filename[FILELEN];  // save ./score_table.csv
	char *ptr;  // save qname
	int i;  // index for loop

	ptr = malloc(sizeof(char) * FILELEN);  // malloc 

	while(1){

		printf("Input question's number to modify >> ");
		scanf("%s", modiName);  // input qname to modify

		if(strcmp(modiName, "no") == 0)  // modify end
			break;

		for(i=0; i < sizeof(score_table) / sizeof(score_table[0]); i++){  // sequential search
			strcpy(ptr, score_table[i].qname);  // copy qname to ptr. qname ex) 1-1.txt
			ptr = strtok(ptr, ".");  // save without extension
			if(!strcmp(ptr, modiName)){  // modiname == cur name
				printf("Current score : %.2f\n", score_table[i].score);
				printf("New score : ");
				scanf("%lf", &newScore);  // input new baejum
				getchar();  // remove \n from buffer
				score_table[i].score = newScore;  // modify newScore
				break;
			}
		}
	}
	// save ./score_table.csv
	/*
	if (snprintf(filename, sizeof(filename), "./%s", score_table_path) >= sizeof(filename))
		fprintf(stderr, "buffer overflow - string is truncated\n");
   */
	// score_table not exists exception	
	if (access(score_table_path, F_OK) < 0) {
		fprintf(stderr, "file doesn't exists %s\n", score_table_path);
		exit(1);
	}
	write_scoreTable(filename);  // modify score_table.csv 
	free(ptr);  // free

}

/* errorDIr 경로 출력 */
void do_eOption()
{
	printf("error saved... %s\n", errorDir);  // errorDIr 경로 출력
}

/* src 배열에 target이 존재하는지 리턴 */
int is_exist(char (*src)[FILELEN], char *target)  
{
	int i = 0;

	while(1)
	{
		if(i >= ARGNUM)  // i가 argnum보다 크거나 같으면 false
			return false;
		else if(!strcmp(src[i], ""))  // src[i]가 NULL이면 false
			return false;
		else if(!strcmp(src[i++], target))  // target과 이름이 같으면 true
			return true;
	}
	return false;
}

// main에서 stuDir, ansDir 경로 저장 후 처음 호출되는 함수
void set_scoreTable(char *ansDir)
{
	char filename[FILELEN];  // score_table.csv 경로 저장, 128 바이트

	// ssu_score이 있는 디렉토리의 score_table.csv 경로 저장
	if (snprintf(filename, sizeof(filename), "%s", score_table_path) >= sizeof(filename))
		fprintf(stderr, "buffer overflow - string is truncated\n");  

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

/* make_scoreTable에서 호출됨 */
/* return : 문제 배점 방식 리턴 */
int get_create_type()
{
	int num;

	while(1)
	{
		printf("score_table.csv file doesn't exist in %s/ANS !\n", currentDir); 
		printf("1. input blank question and program question's score. ex) 0.5 1\n");
		printf("2. input all question's score. ex) Input value of 1-1: 0.1\n");
		printf("select type >> ");
		scanf("%d", &num);   // type 입력

		if(num != 1 && num != 2)  // 잘못된 type 입력
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


	if((fd = creat(score_csv_path, 0666)) < 0){  // score.csv 생성
		fprintf(stderr, "creat error for %s\n", score_csv_path);  // creat 예외 처리
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

	if (cOption)
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
	double score = 0;  // 총점
	int i;  // 반복분의 인덱스
	char tmp[BUFLEN];  // 학생이 제출한 답안의 절대 경로 또는 점수 저장
	int size = sizeof(score_table) / sizeof(score_table[0]);  // 문제 수

	// add ID_node. save STUDENT_ID, student's total score.
	ID_node *id_node = create_id_node(id);
	add_id_node(id_node);

	for(i = 0; i < size ; i++)  // 문제 수 만큼 반복
	{
		if(score_table[i].score == 0)  // 모든 문제 채점이 끝나면 종료
			break;

		// 학번이 id인 학생의 제출 답안의 절대 경로 ex) stuDir/20230000/1-1.txt
		if (snprintf(tmp, sizeof(tmp), "%s/%s/%s", stuDir, id,
					score_table[i].qname) >= sizeof(tmp))
			// 경로 길이가 tmp보다 크다면 예외처리
			fprintf(stderr, "buffer overflow - string is truncated\n");

		if(access(tmp, F_OK) < 0)  // 학생이 제출한 답안이 존재하지 않으면
			result = false;  // 0 저장
		else  // 답안을 제출 했으면
		{
			if((type = get_file_type(score_table[i].qname)) < 0)  // 제출한 파일이 .txt나 .c가 아닌 경우 패스
				continue;

			if(type == TEXTFILE)  // 문제가 .txt 파일
				result = score_blank(id, score_table[i].qname);  // .txt 문제 채점 후 정답 여부 저장
			else if(type == CFILE)  // 문제가 .c 파일
				result = score_program(id, score_table[i].qname); // .c 문제 채점 후 정답 여부 저장
		}

		// make linked list
		Q_node *q_node;

		if(result == false) { // 학생의 답안이 오답이라면
			write(fd, "0,", 2);  // 0점 처리
			q_node = create_q_node(score_table[i].qname, 0.0, score_table[i].score);
		}
		else{  // 오답이 아니라면
			if(result == true){  // 정답이라면
				score += score_table[i].score;  // i번째 문제의 배점을 총점에 더하기
				sprintf(tmp, "%.2f,", score_table[i].score);  // tmp에 배점 저장
				q_node = create_q_node(score_table[i].qname, score_table[i].score, score_table[i].score);
			}
			else if(result < 0){  // 감점을 고려해서 감점 값인 음수를 리턴
				score = score + score_table[i].score + result;  // 총점 + 배점 - 감점
				sprintf(tmp, "%.2f,", score_table[i].score + result);  // tmp에 총점 저장
				q_node = create_q_node(score_table[i].qname, score_table[i].score + result, score_table[i].score);
			}

			write(fd, tmp, strlen(tmp));  // score.csv에 학생의 점수 쓰기
		}
		add_q_node(id_node, q_node);
	}

	if (cOption && pOption) {
		// if id in iIDs or c_opt_students
		if (in_iIDs(id) || in_c_students(id)) {
			printf("%s is finished.. score : %.2f, wrong problem : ", id, score);
			print_pOption(id);
		} else {
			printf("%s is finished..\n", id);
		}
	} else if (cOption) {
		if (strlen(c_opt_students[0]) == 0 || print_with_score(id)) {
			printf("%s is finished.. score : %.2f\n", id, score);  // id 학생의 총점 출력
		}
		else {
			printf("%s is finished..\n", id);  // grading complete 
		}
	} else if (pOption) {
		printf("%s is finished..", id);  // id 학생의 총점 출력

		if (in_iIDs(id)) {	  // print linked list
			printf(" wrong problem :"); 
			print_pOption(id);
		}
		else {
			printf("\n");
		}
	} else
		printf("%s is finished..\n", id);  // grading complete 

	// save student's total score
	id_node->score = score;

	sprintf(tmp, "%.2f\n", score);  // tmp에 문자열로 총점 저장
	write(fd, tmp, strlen(tmp));  // 총점 score.csv에 쓰기

	return score;  // id 학생의 총점 리턴
}

/* score_students()에서 호출됨 */
/* score.csv의 첫 행에 문제 번호들, 합계를 적는다. */ 
/* ,1-1.txt,1-2.txt,...,29.c,sum */
/* fd : score.csv */
void write_first_row(int fd)
{
	int i;
	char tmp[BUFLEN];
	int size = sizeof(score_table) / sizeof(score_table[0]);  // 문제들 수

	write(fd, ",", 1);  // , 쓰기

	for(i = 0; i < size; i++){
		if(score_table[i].score == 0)  // 전부 다 썼다면 종료
			break;

		sprintf(tmp, "%s,", score_table[i].qname);  // 문제 이름에 , 붙이기
		write(fd, tmp, strlen(tmp));  // score.csv에 쓰기
	}
	write(fd, "sum\n", 4);  // 마지막으로 sum 쓰기
}

/* score_blank()에서 호출됨 */
/* fd : 학생 또는 답안파일의 fd, result : 제출한 답안 저장 */
/* 모범 답안 파일의 경우 같은 fd에 대해 또 호출된다면 */
/* : 를 기준으로 다음 답을 저장 */
/* return : 제출한 답안 */
char *get_answer(int fd, char *result)
{
	char c;  // 1 바이트씩 읽기
	int idx = 0;  // index for result

	memset(result, 0, BUFLEN);  // result 초기화
	while(read(fd, &c, 1) > 0)  // 1 바이트씩 읽기
	{
		if(c == ':')  // :를 읽으면 종료
			break;

		result[idx++] = c;  // result에 읽은 글자 옮기기
	}
	if(result[strlen(result) - 1] == '\n')  // \n을 \0 으로 변환
		result[strlen(result) - 1] = '\0';

	return result;  // 읽은 답안 리턴
}

/* 빈칸 채우기 문제 채점 */
/* id가 제출한 filename의 답을 트리로 만들고 */
/* filename의 모범 답안을 트리로 만들어 비교 */
/* id : 학번, filename : 채점 할 파일명 ex) 1-1.txt */
/* return : true - 정답, false - 오답 */
int score_blank(char *id, char *filename)
{
	char tokens[TOKEN_CNT][MINLEN];  // 답안을 쪼갠다. 50개 토큰, 최대 길이 : 64
	node *std_root = NULL, *ans_root = NULL;  // student, answer 트리의 root
	int idx, start;
	char tmp[BUFLEN];
	char s_answer[BUFLEN], a_answer[BUFLEN];  // 학생 답안, 모범 답안
	char qname[FILELEN];
	int fd_std, fd_ans;  // fd for 학생 파일, 답안 파일
	int result = true;  // 정답 결과
	int has_semicolon = false;  // 답안의 ; 여부

	memset(qname, 0, sizeof(qname));  // qname을 0으로 초기화
									  // 문제 이름에서 확장자 제거 ex) 1-1.txt -> 1.1
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));
	// 학생이 제출한 filename의 절대 경로
	if (snprintf(tmp, sizeof(tmp), "%s/%s/%s", stuDir, id, filename) >= sizeof(tmp))
		fprintf(stderr, "buffer overflow - string is truncated\n");
	fd_std = open(tmp, O_RDONLY);  // 학생 제출 파일 열기
	strcpy(s_answer, get_answer(fd_std, s_answer));  // s_answer에 학생 답안 저장

	if(!strcmp(s_answer, "")){  // 빈칸을 써서 냈다면
		close(fd_std); // 파일 닫기
		return false;  // 오답
	}

	if(!check_brackets(s_answer)){  // 답안의 ( ) 짝이 안맞으면
		close(fd_std);  // 파일 닫기
		return false;  // 오답
	}

	strcpy(s_answer, ltrim(rtrim(s_answer)));  // 답안의 좌우 공백 제거

	if(s_answer[strlen(s_answer) - 1] == ';'){  // 답안이 ;로 끝나면
		has_semicolon = true;  // ; flag on
		s_answer[strlen(s_answer) - 1] = '\0';  // ;를 \0으로 바꾸기
	}

	if(!make_tokens(s_answer, tokens)){  // 답안을 쪼개서 tokens에 저장
		close(fd_std);
		return false;  // 오답
	}

	idx = 0;
	// 답안 토큰들로 학생 트리 생성
	std_root = make_tree(std_root, tokens, &idx, 0);  // tokens로 만든 트리의 root를 가리킴

	// tmp에 답안 파일의 절대 경로 저장
	if (snprintf(tmp, sizeof(tmp), "%s/%s", ansDir, filename) >= sizeof(tmp))
		fprintf(stderr, "buffer overflow - string is truncated\n");
	fd_ans = open(tmp, O_RDONLY);  // 답안 파일 열기

	// 학생 답안과 모범 답안을 비교하는데,
	// 각 답안을 tokens로 바꾸고 트리를 만들어 트리끼리 비교한다.
	while(1)
	{
		ans_root = NULL;
		result = true;

		for(idx = 0; idx < TOKEN_CNT; idx++)  // 0 ~ 49
			memset(tokens[idx], 0, sizeof(tokens[idx]));  // tokens 초기화

		strcpy(a_answer, get_answer(fd_ans, a_answer));  // a_answer에 모범 답안 복사

		if(!strcmp(a_answer, ""))  // a_answer가 공백이면 break;
		break;

		strcpy(a_answer, ltrim(rtrim(a_answer)));  // a_answer의 좌우 공백 제거

		if(has_semicolon == false){  // 학생 답안에 ;가 없는데
			if(a_answer[strlen(a_answer) -1] == ';')  // 모범 답안의 마지막 글자가 ; 이면
				continue;  // 패스
		}

		else if(has_semicolon == true)  // 학생 답안에 ;가 있으면
		{
			if(a_answer[strlen(a_answer) - 1] != ';')  // 모범 답안의 끝이 ;면
				continue;
			else
				a_answer[strlen(a_answer) - 1] = '\0';  // 학생처럼 \0로 바꾸기
		}

		if(!make_tokens(a_answer, tokens))  // 모범 답안으로 tokens 채우기
			continue;

		idx = 0;
		ans_root = make_tree(ans_root, tokens, &idx, 0);  // 트리 생성

		compare_tree(std_root, ans_root, &result);  // 비교 결과가 result에 저장

		if(result == true){  // 정답이라면 
			close(fd_std);  // 학생 파일 닫기
			close(fd_ans);  // 정답 파일 닫기

			if(std_root != NULL)      // 트리에 노드가 남아 있다면
				free_node(std_root);  // 트리 노드 해제
			if(ans_root != NULL)      // 트리에 노드가 남아 있다면
				free_node(ans_root);  // 트리 노드 해제
			return true;  // 정답

		}
	}

	close(fd_std);  // 학생 파일 닫기
	close(fd_ans);  // 정답 파일 닫기

	if(std_root != NULL)      // 트리에 노드가 남아 있다면
		free_node(std_root);  // 트리 노드 해제
	if(ans_root != NULL)      // 트리에 노드가 남아 있다면
		free_node(ans_root);  // 트리 노드 해제

	return false;  // 오답
}

/* 프로그램 문제 채점 */
/* id가 제출한 filename.c를 컴파일하여 .stdexe를 만들고 실행 결과를.stdout에 저장하고 */
/* 답안 filename.c를 컴파일하여 .exe를 만들고 실행 결과를.stdout에 저장하여 */
/* 각 .stdout을 한 문자씩 비교 */
/* id : 학번, filename : 채점 할 파일명 ex) 1-1.txt */
/* return : true(정답), false(오답) */
double score_program(char *id, char *filename)
{
	double compile;  // 컴파일 에러 여부 저장 or
	int result;

	compile = compile_program(id, filename);  // 컴파일 성공시 1, 에러시 0

	if(compile == ERROR || compile == false)  // 컴파일 에러시 오답 리턴
		return false;

	result = execute_program(id, filename);  // 정답 여부 저장

	if(!result)  // 오답
		return false;

	if(compile < 0)  
		return compile;

	return true;  // 정답
}

/* compile_program에서 호출됨 */
/* threadFiles 배열에 qname이 있는지 리턴 */
/* qname : 확장자 없는 파일 명 ex) 20.c의 20*/
int is_thread(char *qname)
{
	int i;
	int size = sizeof(threadFiles) / sizeof(threadFiles[0]);  // threadFiles의 크기

	for(i = 0; i < size; i++){
		if(!strcmp(threadFiles[i], qname))  // threadFiles에 qname이 있다면 true 리턴
			return true;
	}
	return false;  // 없으면 false 리턴
}

/* score_program()에서 호출됨 */
/* 학생과 답안 .c 파일로 exe, error.txt 파일 생성 */
/* id : 학번, filename : 채점 할 파일명 ex) 1-1.txt */
/* return : 실수(e 옵션), true(컴파일 시 에러 x), false(컴파일 시 에러 o) */
double compile_program(char *id, char *filename)
{
	int fd;  // creat 용 fd
	char tmp_f[BUFLEN], tmp_e[BUFLEN];  // filename의 절대 경로, qname_error.txt의 절대 경로
	char command[BUFLEN];  // gcc가 담길 명령어
	char qname[FILELEN];  // 확장자 없는 채점할 파일의 이름 ex) 20
	int isthread;  // add -lpthread option in gcc
	off_t size;  // error.txt의 크기
	double result;

	memset(qname, 0, sizeof(qname));  // qname 초기화
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));  // filename에서 확장자 제거


	isthread = is_thread(qname);  // 스레드로 작업중인지 확인

	// tmp_f에 정답 파일(.c)의 절대 경로 저장
	if (snprintf(tmp_f, sizeof(tmp_f), "%s/%s", ansDir, filename) >= sizeof(tmp_f))
		fprintf(stderr, "buffer overflow - string is truncated\n");
	// tmp_e에 정답 실행파일(.exe)의 절대 경로 저장.
	// ./ANS/qname.exe
	if (snprintf(tmp_e, sizeof(tmp_e), "%s/%s.exe", ANS_Dir, qname) >= sizeof(tmp_e))
		fprintf(stderr, "buffer overflow - string is truncated\n");

	if(tOption) {  // t : -lpthread option
		if (strlen(threadFiles[0]) == 0 || isthread) { // only -t or some files -lpthread
													   // command에 컴파일 명령 넣기. -lpthread : pthread 라이브러리 사용 시 컴파일 옵션
			if (snprintf(command, sizeof(command), "gcc -o %s %s -lpthread", tmp_e, tmp_f) >= sizeof(command))
				fprintf(stderr, "buffer overflow - string is truncated\n");
		} else {
			// command에 컴파일 명령 넣기
			if (snprintf(command, sizeof(command), "gcc -o %s %s", tmp_e, tmp_f) >= sizeof(command))
				fprintf(stderr, "buffer overflow - string is truncated\n");
		}
	}
	else {
		// command에 컴파일 명령 넣기
		if (snprintf(command, sizeof(command), "gcc -o %s %s", tmp_e, tmp_f) >= sizeof(command))
			fprintf(stderr, "buffer overflow - string is truncated\n");
	}


	// tmp_e에 qname_error.txt 저장
	if (snprintf(tmp_e, sizeof(tmp_e), "%s/%s_error.txt", ANS_Dir, qname) >= sizeof(tmp_e))
		fprintf(stderr, "buffer overflow - string is truncated\n");
	fd = creat(tmp_e, 0666);  // ANS_DIR 아래에 20_error.txt 생성. 

	// ansDir/qname.exe 생성, command의 표준에러를 error.txt에 출력
	redirection(command, fd, STDERR);
	size = lseek(fd, 0, SEEK_END);  // error.txt의 파일 크기 저장
	close(fd);  // 파일 닫기
	unlink(tmp_e);  // error.txt 삭제

	if(size > 0)  // 에러가 출력 되었다면 오답
		return false;

	// tmp_f에 id 학생이 제출한 filename 절대 경로 저장
	if (snprintf(tmp_f, sizeof(tmp_f), "%s/%s/%s", stuDir, id, filename) >= sizeof(tmp_f))
		fprintf(stderr, "buffer overflow - string is truncated\n");
	// tmp_e에 id 학생이 제출한 filename의 .stdexe 절대 경로 저장
	if (snprintf(tmp_e, sizeof(tmp_e), "%s/%s/%s.stdexe", STD_Dir, id, qname) >= sizeof(tmp_e))
		fprintf(stderr, "buffer overflow - string is truncated\n");

	// t 옵션
	if(tOption) {
		if (strlen(threadFiles[0]) == 0 || isthread) { // only -t or some files -lpthread
													   // command에 컴파일 명령 넣기. -lpthread : pthread 라이브러리 사용 시 컴파일 옵션
			if (snprintf(command, sizeof(command), "gcc -o %s %s -lpthread", tmp_e, tmp_f) >= sizeof(command))
				fprintf(stderr, "buffer overflow - string is truncated\n");
		}
	}
	else
		// command에 컴파일 명령 넣기
		if (snprintf(command, sizeof(command), "gcc -o %s %s", tmp_e, tmp_f) >= sizeof(command))
			fprintf(stderr, "buffer overflow - string is truncated\n");


	// save STD_Dir/id
	char STD_ID_Dir[BUFLEN];
	if (snprintf(STD_ID_Dir, sizeof(STD_ID_Dir), "%s/%s", STD_Dir, id) >= sizeof(STD_ID_Dir))
		fprintf(stderr, "buffer overflow - string is truncated\n");

	if(access(STD_ID_Dir, F_OK) < 0)  // ./STD/id not exists, mkdir
		mkdir(STD_ID_Dir, 0755);

	// tmp_f에 id 학생의 qname_error.txt 저장
	if (snprintf(tmp_f, sizeof(tmp_f), "%s/%s/%s_error.txt", STD_Dir, id, qname) >= sizeof(tmp_f))
		fprintf(stderr, "buffer overflow - string is truncated\n");
	fd = creat(tmp_f, 0666);  // stuDir/id/qname_error.txt 저장

	redirection(command, fd, STDERR);  // command의 표준 에러를 error.txt에 출력
	size = lseek(fd, 0, SEEK_END);  // error.txt 파일 크기 저장
	close(fd);  // 파일 닫기

	if(size > 0){  // 에러가 있다면
		if(eOption)
		{
			// to tmp_e save errorDir/id
			if (snprintf(tmp_e, sizeof(tmp_e), "%s/%s", errorDir, id) >= sizeof(tmp_e))
				fprintf(stderr, "buffer overflow - string is truncated\n");
			if(access(tmp_e, F_OK) < 0)  // mkdir errorDir/id 
				mkdir(tmp_e, 0755);

			// to tmp_e, save errorDir/id/qname_error.txt
			if (snprintf(tmp_e, sizeof(tmp_e), "%s/%s/%s_error.txt", errorDir, id, qname) >= sizeof(tmp_e))
				fprintf(stderr, "buffer overflow - string is truncated\n");
			rename(tmp_f, tmp_e); // move created stdDir/id/qname_error.txt to id's errorDir

			result = check_error_warning(tmp_e);  // gamjum or wrong 
		}
		else{ 
			result = check_error_warning(tmp_f);  // gamjum or wrong 
			unlink(tmp_f);  // not -e option, remove qname_error.txt
		}

		return result;  // return graded score
	}

	unlink(tmp_f);  // stuDir/id/20_error.txt 삭제
	return true;  // 에러가 없으므로 true 리턴
}

/* called from compile_program() */
/* gamjum by 1 warn */
/* filename : errorDir/id/qname_error.txt */ 
/* return : 0(wrong) or minus(gamjum)*/ 
double check_error_warning(char *filename)
{
	FILE *fp;
	char tmp[BUFLEN];
	double warning = 0;

	if((fp = fopen(filename, "r")) == NULL){  // file open
		fprintf(stderr, "fopen error for %s\n", filename);  // catch exeception
		return false;  // wrong
	}

	while(fscanf(fp, "%s", tmp) > 0){  // save first word in filename to tmp
		if(!strcmp(tmp, "error:"))  // if error, return ERROR(0)
			return ERROR;  // wrong
		else if(!strcmp(tmp, "warning:"))  // if warn, -0.1 by warn
			warning += WARNING;  // -0.1 * num of warn
	}

	return warning;  // gamjum
}

/* score_program()에서 호출됨 */
/* compile_program()에서 만든 qname.exe과 qname.stdexe를 */
/* 실행한 결과 출력을 qname.stdout에 저장한다 */
/* id : 학번, filename : 채점 할 파일명 ex) 1-1.txt */
/* return : true(정답), false(오답 or 실행 시간이 5초 이상)*/
int execute_program(char *id, char *filename)
{
	char std_fname[BUFLEN], ans_fname[BUFLEN];  // qname.stdexe와 qname.exe의 실행 결과 저장할 파일 경로
	char tmp[BUFLEN];  // tmp
	char qname[FILELEN];  // 확장자 없는 filename 저장
	time_t start, end;  // 시간 저장
	pid_t pid;  // qname.stdexe가 백그라운드에서 실행중인지 여부 저장
	int fd;  // creat 용 fd

	memset(qname, 0, sizeof(qname));  // qname 초기화
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));  // filename에서 확장자 제거 후 저장

	// ans_fname에 qname.stdout의 절대 경로 저장
	if (snprintf(ans_fname, sizeof(ans_fname), "%s/%s.stdout", ANS_Dir, qname) >= sizeof(ans_fname))
		fprintf(stderr, "buffer overflow - string is truncated\n");
	fd = creat(ans_fname, 0666);  // 답안 qname.stdout 생성

	// ans_fname에 모범 답안의 qname.exe의 절대 경로 저장
	if (snprintf(tmp, sizeof(tmp), "%s/%s.exe", ANS_Dir, qname) >= sizeof(tmp))
		fprintf(stderr, "buffer overflow - string is truncated\n");
	redirection(tmp, fd, STDOUT);  // qname.exe 실행 결과 출력을 qname.stdout에 저장
	close(fd);  // qname.stdout 닫기

	// std_fname에 학생의 qname.stdout의 절대 경로 저장
	if (snprintf(std_fname, sizeof(std_fname), "%s/%s/%s.stdout", STD_Dir, id, qname) >= sizeof(std_fname))
		fprintf(stderr, "buffer overflow - string is truncated\n");
	fd = creat(std_fname, 0666);  // 학생 qname.stdout 생성

	// tmp에 백그라운드로 학생의 qname.stdexe 실행 저장
	if (snprintf(tmp, sizeof(tmp), "%s/%s/%s.stdexe &", STD_Dir, id, qname) >= sizeof(tmp)) 
		fprintf(stderr, "buffer overflow - string is truncated\n");

	start = time(NULL);  // 실행 시작 시간 저장
	redirection(tmp, fd, STDOUT);  // 학생의 실행 프로그램 실행 결과를 qname.stdout에 저장

	// tmp에 qname.stdexe 저장
	if (snprintf(tmp, sizeof(tmp), "%s.stdexe", qname) >= sizeof(tmp))
		fprintf(stderr, "buffer overflow - string is truncated\n");
	while((pid = inBackground(tmp)) > 0){  // qname.stdexe가 백그라운드로 실행중이라면
		end = time(NULL);  // 현재 시간 저장

		if(difftime(end, start) > OVER){  // 5초 이상 걸리면 
			kill(pid, SIGKILL);  // 프로세스 kill
			close(fd);  // 파일 닫기
			return false;
		}
	}

	close(fd);

	return compare_resultfile(std_fname, ans_fname);
}

/* execute_program() 에서 호출됨 */
/* name : 백그라운드에서 실행중인 파일 이름 */
/* return : 백그라운드로 실행중인 pid 리턴, 없으면 0 */
pid_t inBackground(char *name)
{
	pid_t pid;
	char command[64];
	char tmp[64];
	int fd;
	off_t size;

	memset(tmp, 0, sizeof(tmp));  // tmp 초기화
	fd = open("background.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);  // background.txt 생성

	sprintf(command, "ps | grep %s", name);  // 프로세스 찾는 명령어
	redirection(command, fd, STDOUT);  // ps | grep 의 결과를 background.txt에 출력

	lseek(fd, 0, SEEK_SET);  // background.txt의 맨 앞으로 오프셋 이동
	read(fd, tmp, sizeof(tmp));  // 64바이트 읽기

	if(!strcmp(tmp, "")){  // ps 의 결과가 없다면
		unlink("background.txt");  // 파일 삭제
		close(fd);  // 파일 닫기
		return 0;
	}

	pid = atoi(strtok(tmp, " "));  // PID 번호를 int로 저장
	close(fd);  // 파일 닫기

	unlink("background.txt");  // 파일 삭제
	return pid;  // pid 리턴
}

/* execute_program() 에서 호출됨 */
/* file1 : 학생 프로그램 실행 결과 */
/* file2 : 정답 프로그램 실행 결과 */
/* return : true(정답), false(오답) */
int compare_resultfile(char *file1, char *file2)
{
	int fd1, fd2;  // 파일 용 fd
	char c1, c2;  // 문자 저장
	int len1, len2;  // 파일 길이

	fd1 = open(file1, O_RDONLY);  // stuDir/id/20.stdout 열기
	fd2 = open(file2, O_RDONLY);  // ansDir/20.stdout 열기

	while(1)
	{
		while((len1 = read(fd1, &c1, 1)) > 0){  // 읽은 문자가 있다면
			if(c1 == ' ')  // 공백이면 패스
				continue;
			else 
				break;
		}
		while((len2 = read(fd2, &c2, 1)) > 0){  // 읽은 문자가 있다면
			if(c2 == ' ')  // 공백이면 패스
				continue;
			else 
				break;
		}

		if(len1 == 0 && len2 == 0)  // 둘 다 실행 결과가 없으면 종료
			break;

		to_lower_case(&c1);  // 소문자로 변환
		to_lower_case(&c2);  // 소문자로 변환

		if(c1 != c2){  // 읽은 문자가 다르면 오답
			close(fd1);  // 파일 닫기
			close(fd2);  // 파일 닫기
			return false;  // 오답
		}
	}
	close(fd1);  // 파일 닫기
	close(fd2);  // 파일 닫기
	return true;  // 정답
}

/* compile_program()에서 호출됨 ex) gcc -o 20_error.txt 20.c -lpthread */
/* system(command)는 gcc라 수정 아닐듯. */
/* old에 출력되는 command 수행 내용을 new에 출력한다. ex) 컴파일 에러를 error.txt에 쓰기 */
/* command : 명령어, new : fd, old : STDERR */
void redirection(char *command, int new, int old)
{
	int saved;

	saved = dup(old);  // old가 가리키는 파일 구조체 저장
	dup2(new, old);  // old가 new를 가리킴. STDERR가 error.txt를 가리킴

	system(command);  // gcc 에러는 표준에러 STDERR->error.txt에 출력

	dup2(saved, old);  // old 복구
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

/* 디렉토리들 제거 */
void rmdirs(const char *path)
{
	struct dirent *dirp;
	struct stat statbuf;
	DIR *dp;
	//char tmp[50]; 
	char tmp[4096];  // path max length = 4096

	if((dp = opendir(path)) == NULL)  // 열리지 않으면 리턴
		return;

	while((dirp = readdir(dp)) != NULL)
	{
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))  // .이나 ..은 패스
			continue;

		sprintf(tmp, "%s/%s", path, dirp->d_name);  // 절대 경로 저장

		if(lstat(tmp, &statbuf) == -1)  // stat 에러시 continue;
			continue;

		if(S_ISDIR(statbuf.st_mode))  // 디렉토리의 경우 재귀적으로 하위 요소 제거
			rmdirs(tmp);
		else
			unlink(tmp);  // 일반 파일 삭제
	}

	closedir(dp);
	rmdir(path);  // 하위 파일들 제거 후 path 디렉토리 제거
}

/* c 문자를 소문자로 변경 */
void to_lower_case(char *c)
{
	if(*c >= 'A' && *c <= 'Z')
		*c = *c + 32;
}

/* usage 출력 */
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

/* path를 절대 경로로 변경 */
char *to_abs_path(char *path) {
	if (path[0] == '/')  // path is already abs_path
		return path;

	char *buf = (char *) malloc(BUFLEN);
	getcwd(buf, BUFLEN);  // cwd 저장
	strcat(buf, "/");  // '/' 붙이기
	strcat(buf, path);  // 상대 경로 붙이기
	return buf;
}

/* ANS, STD 디렉토리 생성 */
void create_ANS_STD_Dir() {
	getcwd(currentDir, BUFLEN);  // 현재 디렉토리 경로 저장
	// 경로 생성
	strcpy(ANS_Dir, currentDir);  
	strcat(ANS_Dir, "/ANS");
	strcpy(STD_Dir, currentDir);
	strcat(STD_Dir, "/STD");

	if(access(ANS_Dir, F_OK) < 0)  // ./ANS not exists, mkdir
		mkdir(ANS_Dir, 0755);

	if(access(STD_Dir, F_OK) < 0)  // ./STD not exists, mkdir
		mkdir(STD_Dir, 0755);
}	

/* 점수와 함께 출력할지 여부 리턴 */
int print_with_score(char *id) {
	// find id in c_opt_students
	for (int i=0; i<ARGNUM; i++)
		if (strcmp(c_opt_students[i], id) == 0)
			return 1;

	// cannot find
	return 0;
}

/* called from check_option's case c */
/* IDlist : array to save STUDENT_ID in STD_DIR */
/* return : num of students in STD_DIR */
int getStudentIDs(char (*IDlist)[10]) {
	int num = 0;

	DIR *dir;
	struct dirent *dirp;
	struct stat statbuf;

	if ((dir = opendir(stuDir)) == NULL) {
		fprintf(stderr, "opendir error for %s\n", stuDir);
		exit(1);
	}

	while ((dirp = readdir(dir)) != NULL) {  // stdDir 하위 파일 순회
		if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))  // .이나 ..은 continue;
			continue;

		// 절대 경로 생성
		char fullpath[BUFLEN];  
		memset(fullpath, 0, BUFLEN);
		strcat(fullpath, stuDir);
		strcat(fullpath, "/");
		strcat(fullpath, dirp->d_name);

		if (lstat(fullpath, &statbuf) < 0) {
			fprintf(stderr, "lstat error for %s\n", fullpath);
			exit(1);
		}
		if (S_ISDIR(statbuf.st_mode)) {  // 학번을 IDlist에 저장
			strcpy(IDlist[num++], dirp->d_name);
		}
	}
	return num;  // 학생 수 저장
}

/* 학번 기준으로 id_table 정렬 */
/* size : 학생 수 */
void sort_2Darray(char (*arr)[10], int size)  
{
	int i, j;  // 정렬에 쓰이는 인덱스
	char tmp[10];  // 학번 임시 저장

	for(i = 0; i < size - 1; i++){
		for(j = 0; j < size - 1 -i; j++){
			if(strcmp(arr[j], arr[j+1]) > 0){  // j의 학번이 더 높다면
				strcpy(tmp, arr[j]);  // j의 학번을 tmp에 저장
				strcpy(arr[j], arr[j+1]);  // j+1의 학번을 j에 저장
				strcpy(arr[j+1], tmp);  // tmp의 학번을 j+1에 저장
			}
		}
	}
}

/* called from score_student */
/* print qname(baejum) by id */
/* id : STUDENT_ID */
void print_pOption(char *id) {
	ID_node *parent = find_node_by_id(id);  // ID 학생의 ID_node 가져오기

	if (parent->child == NULL) return;

	Q_node *cur = parent->child;  // 첫 문제 노드 가리키기

	while (cur->next != NULL) {
		if (cur->result == 0)
			printf("%s(%lg), ", cur->qname, cur->score);  // 틀린 문제 출력
		cur = cur->next;  // 다음 노드 가리키기
	}
	if (cur->result == 0)  // 마지막 틀린 노드 출력
		printf("%s(%lg)\n", cur->qname, cur->score);
}

/* ID_node 생성 후 리턴*/
ID_node *create_id_node(char *id) {
	ID_node *node = (ID_node *) malloc(sizeof (ID_node));
	strcpy(node->id, id);  // 학번 저장
	node->child = NULL;
	node->next = NULL;
	node->sorted = 0;  // 정렬 되었는지 여부 저장

	return node;
}

/* ID_node를 연결 리스트에 추가 */
void add_id_node(ID_node *new) {
	if (pHEAD == NULL) {  // 첫 노드의 경우
		pHEAD = new;
		pREAR = new;
		return;
	}

	pREAR->next = new;  // 마지막 노드의 다음에 추가한다.
	pREAR = new;  // pREAR가 새 노드를 가리킨다.
}

/* Q_node 생성 후 리턴 */
Q_node *create_q_node(char *qname, double result, double score) {
	Q_node *node = (Q_node *) malloc(sizeof(Q_node));
	node->result = result;  // 채점 결과 저장
	node->score = score;  // 배점 저장
	strcpy(node->qname, qname);  // 문제 이름 저장
	node->next = NULL;

	return node;	
}

/* ID_node의 자식으로 맨 뒤에 추가 */
void add_q_node(ID_node *parent, Q_node *new) {
	if (parent->child == NULL) {  // 첫 노드의 경우
		parent->child = new;
		return;
	}

	Q_node *cur = parent->child;

	while (cur->next != NULL)  // 마지막으로 노드 이동
		cur = cur->next;  // 다음 노드 가리키기

	cur->next = new;  // 노드 추가
}

/* ID_node 리스트에서 id기반 노드 찾기 */
ID_node *find_node_by_id(char *id) {
	ID_node *cur = pHEAD;

	while (cur != NULL) {
		if (!strcmp(cur->id, id))  // 현재 노드의 id가 id이면 리턴
			return cur;
		cur = cur->next;  // 다음 노드 가리키기
	}

	return NULL;
}

/* id가 존재하는 학번인지 여부 리턴 */
int in_iIDs(char *id) {
	for (int i=0; i<SNUM; i++) {
		if (!strcmp(iIDs[i], id))  // iIDs에서 id 찾기
			return 1;
	}
	return 0;
}

/* id가 c옵션 STD_ID에 있는지 여부 리턴 */
int in_c_students(char *id) {
	for (int i=0; i<ARGNUM; i++) {
		if (!strcmp(c_opt_students[i], id))  // c_opt_students에서 id 찾기
			return 1;
	}
	return 0;
}

/* s 옵션 시 연결리스트 정렬 후 */
/* 순회하며 score.csv에 쓰기 */
void do_sOption() {
	sort_linked_list();
	iter_sorted_nodes();
}

/* ID_node 연결 리스트 정렬 */
void sort_linked_list() {
	int i = 0;
	ID_node *iter = pHEAD;
	while (iter != NULL) {  // 노드 수 만큼 순회
		ID_node *cur = pHEAD;  // 순회에 쓰이는 노드
		ID_node *tmp = pHEAD;  // 조건을 만족하는 노드를 가리킴

		while (cur != NULL && cur->sorted)  // sorted 노드 건너뛰기
			cur = cur->next;
		tmp = cur;  // 첫 sorted가 아닌 노드
		if (!strcmp(category, "stdid")) {  // 카테고리가 stdid이면
			if (is_ASC == 1) {  // 오름차순

				while (cur != NULL) {  // find least id node
					if (cur->sorted) {  // sorted node 건너뛰기
						cur = cur->next;  // cur 저장
						continue;
					}

					if (strcmp(cur->id, tmp->id) <= 0)  // 현재 노드의 id가 작다면
						tmp = cur;  // cur 저장
					cur = cur->next;  // 다음 노드 가리키기
				}

				tmp->sorted = 1;  // 정렬 표시
				add_id_node2(tmp);  // sorted_list에 추가
			}
			else {  // 내림차순
				while (cur != NULL) {  // find least id node
					if (cur->sorted) {  // sorted node 건너뛰기
						cur = cur->next;  // cur 저장
						continue;
					}

					if (strcmp(cur->id, tmp->id) >= 0)  // 현재 노드의 id가 크다면
						tmp = cur;  // cur 저장
					cur = cur->next;  // 다음 노드 가리키기
				}

				tmp->sorted = 1;  // 정렬 표시
				add_id_node2(tmp);  // sorted_list에 추가
			}
		} else {  // 카테고리가 score이면
			if (is_ASC == 1) {  // 오름차순
				while (cur != NULL) {  // find least id node
					if (cur->sorted) {  // sorted node 건너뛰기
						cur = cur->next;  // 다음 노드 가리키기
						continue;
					}

					if (cur->score <= tmp->score)  // 현재 노드의 총점이 작다면
						tmp = cur;
					cur = cur->next;  // 다음 노드 가리키기
				}

				tmp->sorted = 1;  // 정렬 표시
				add_id_node2(tmp);  // sorted_list에 추가
			}
			else {  // 내림차순
				while (cur != NULL) {  // find least id node
					if (cur->sorted) {  // sorted node 건너뛰기
						cur = cur->next;  // 다음 노드 가리키기
						continue;
					}

					if (cur->score >= tmp->score)  // 현재 노드의 총점이 크다면
						tmp = cur;
					cur = cur->next;  // 다음 노드 가리키기
				}

				tmp->sorted = 1;  // 정렬 표시
				add_id_node2(tmp);  // sorted_list에 추가
			}
		}
		iter = iter->next;  // 다음 노드 가리키기
	}
}

/* sorted_list에 노드 추가 */
void add_id_node2(ID_node *new) {
	sorted_node *cur = (sorted_node *) malloc(sizeof(sorted_node));  // new를 가리키는 노드 생성
	cur->child = new;  // cur의 자식으로 new 가리키기
	if (sHEAD == NULL) {  // 첫 노드이면
		sHEAD = cur;
		sREAR = cur;
		cur->next = NULL;
		return;
	}

	sREAR->next = cur;  // sREAR의 다음 노드로 cur 추가
	sREAR = cur;  // cur을 sREAR이 가리킴
}

/* 정렬된 sorted_list 순회하며 score.csv 작성 */
void iter_sorted_nodes() {
	sorted_node *cur = sHEAD;

	// score.csv 다시 열기
	int fd;
	if ((fd = open(score_csv_path, O_CREAT | O_WRONLY | O_TRUNC, 0666)) < 0) {
		fprintf(stderr, "open error for %s\n", score_csv_path);
		exit(1);
	}

	char tmp[BUFLEN];  // 학번 저장용 tmp

	write_first_row(fd);  // score.csv에 첫번째 행 추가. 문제 번호들과 합계가 적힘

	while (cur != NULL) {
		memset(tmp, 0, BUFLEN);  // tmp 초기화
		sprintf(tmp, "%s,", cur->child->id);  // 학번, 저장
		write(fd, tmp, strlen(tmp));  // score.csv에 "학번," 쓰기
		rewrite_score_csv(fd, cur->child);  // 문제 채점 결과를 쓰기
		cur = cur->next;  // 다음 노드 가리키기
	}
	close(fd);  // close file
}

/* parent가 가진 q_node들의 내용을 score.csv에 쓰기 */
void rewrite_score_csv(int fd, ID_node *parent) {
	Q_node *cur = parent->child;

	char tmp[BUFLEN];  // 점수 저장용 배열

	while (cur != NULL) {
		sprintf(tmp, "%.2f,", cur->result);  // 점수, 저장
		write(fd, tmp, strlen(tmp));  // score.csv에 점수 쓰기
		cur = cur->next;  // 다음 노드 가리키기
	}
	sprintf(tmp, "%.2f\n", parent->score);  // tmp에 문자열로 총점 저장
	write(fd, tmp, strlen(tmp));  // 총점 score.csv에 쓰기
}
