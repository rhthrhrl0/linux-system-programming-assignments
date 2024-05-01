#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include<sys/wait.h>
#include"customHeader.h"

#define ARGMAX 5    //프롬프트에서 받을 수 있는 인자 개수
#define BUFMAX 1024 //한 줄 입력 크기
#define PATHMAX 4097 //경로길이



int split(char* string, char* seperator, char* argv[]); //프롬프트입력을 잘라줌.
void command_fmd5(int argc, char* argv[]);
void command_fsha1(int argc, char* argv[]);
void command_help();


//입력받은 바이트들에 대해서 옳은 입력인지 검증하고 어떤 상태인지를 리턴해줌.
int isInputByteWave(char* input){
	if(input[0]=='~') 
	{
		if(strlen(input)==1)
						return 1;
		else
				return -1; //입력 잘못받음.
	}
	else if(strlen(input)>=1 && (int)input[0]>=48 && (int)input[0]<=57)
	{ // ~가 아니라면 첫글자는 무조건 정수여야 함. 
		return 0;
	}
	else
	{
		return -1;
	}
}



int main(){
	//쓰레기통 만들기. 기존에 있으면 그냥 넘어가면 됨.
	mkdir("Trash",0777);

	int argc = 0;
	char input[BUFMAX]; 
	char* argv[ARGMAX]; //입력을 공백단위로 구분받을 이차원배열.

	while (1) {
		printf("20182580> ");
		fgets(input, sizeof(input), stdin);
		input[strlen(input) - 1] = '\0';
		argc = split(input, " ", argv); //몇개나 공백 단위로 입력받았는지 체크

		if (argc == 0)
				continue;
										
		if (!strcmp(argv[0], "fmd5"))
				command_fmd5(argc, argv);
							
		else if(!strcmp(argv[0],"fsha1"))
				command_fsha1(argc,argv);

		else if (!strcmp(argv[0], "exit")) {
				printf("Prompt End\n");
				break;
		}
		else 
				command_help();

	}
	exit(0);
}

void command_fmd5(int argc, char* argv[]){
	char dirname[PATHMAX];
	struct stat statbuf;
	off_t minByte;
	off_t maxByte;
	char minByteString[20]="no";
	char maxByteString[20]="no";

	// 입력 관련 에러 처리
	if (argc != ARGMAX) { //인자 개수도 못채웠다면
				printf("ERROR: Arguments error\n");
				return;
	}

	char inputPath[PATHMAX];
	//인자로 받은 디렉토리가 옳은가?
	if(argv[4][0]=='~'){ //~dir1을 입력받으면... 이건 에러처리를 해줘야함.
			char* homePath=getenv("HOME");
			if(strlen(argv[4])==1){
				strcpy(inputPath,homePath);
			}
			else if(argv[4][1]=='/'){ // ~이후에 /만 오는게 보장된다면 일단 
					//적어도 경로양식이  틀린건 아님.
				sprintf(inputPath,"%s%s",homePath,argv[4]+1);// /를 포함한 이후.
			}
			else{ //~로 시작하고 길이가 2이상인데 ~이후에 / 가 아닌 경우.
				fprintf(stderr,"error....directoryPath from ~\n");
				return;
			}
	}
	else{
		strcpy(inputPath,argv[4]);
	}

	//받은 디렉토리 경로가 이상하면 다시
	if (realpath(inputPath, dirname) == NULL) { //탐색시작 디렉토리의 절대경로 얻기
				printf("ERROR: Path exist error\n");
				return;
	}
	
	//디렉토리 경로는 이상 없는데 lstat 안열리면 다시.
	if (lstat(dirname, &statbuf) < 0) {
				fprintf(stderr, "lstat error for %s\n", dirname);
				exit(1);
	}
	if (!S_ISDIR(statbuf.st_mode)) {
				printf("ERROR: Path must be directory\n");
				return;
	}

	//확장자에 대해
	if(argv[1][0]=='*'){
		if(strlen(argv[1])==1){
			//오케이
		}
		else if(strlen(argv[1])>=2 && argv[1][1]=='.'){
			//오케이
		}
		else
				return; // * 혹은 *.(확장자) 와 다른거 경우를 걸러냄.
	}
	else{ //첫글자가 *이 아니면 걸러냄.
		return;
	}

	int errorSize=0;
	//인풋으로 받은 크기가 제대로된 입력인지에 대한 상태. ~이면 1을 리턴함.
	//정수로 시작을 안하면 일단 에러처리.
	int state=isInputByteWave(argv[2]);
	if(state<0){
		printf("[MINSIZE] input error\n");
		return;
	}
	else if(state){
		minByte=0;
		strcpy(minByteString,"zero");
	}
	else{
		//소수점이 두번 들어가거나 문자가 중간에 껴있으면 에러처리.
		//하지만 끝에 두자리에는 kb,mb,gb가 올 수 있음.
		//이 조건들을 만족하면 해당하는 값으로 변환해서 리턴해줌.
		minByte=returnByteSize(argv[2],&errorSize);
		if(errorSize){ //변환과정에서 잘못됨.
			printf("[MINSIZE] input error\n");
			return;	
		}
	}
	
	errorSize=0;
	state=isInputByteWave(argv[3]);
	if(state<0){
		printf("[MAXSIZE] input error\n");
		return;
	}
	else if(state){
		maxByte=0;
		strcpy(maxByteString,"infinite");
	}
	else{
		maxByte=returnByteSize(argv[3],&errorSize);
		if(errorSize){ //변환과정에서 잘못됨.
			printf("[MAXSIZE] input error\n");
			return;	
		}
	}


	//여기까지 왔다는 것은 입력이 정상적이란 것.
	pid_t pid=fork(); //자식프로세스면 0임. 부모프로세스면 자식프로세스값을 리턴받음. 즉,양수
	if(pid>0){ //부모프로세스는 잠시 대기
		wait(NULL);
		return; //헬프 명령어 끝나면 다시 프롬프트 띄우기.
	}
	else if(pid==0){ //다렉토리의  절대경로
		execl("./fmd5",argv[0],argv[1],argv[2],argv[3],minByteString,maxByteString,dirname,(char*)0);
		//자식은 죽음. 대신 새로 생긴 프로세스가 그 프로세스id를 이어받음.
	}
}


void command_fsha1(int argc, char* argv[]){
	char dirname[PATHMAX];
	struct stat statbuf;
	off_t minByte;
	off_t maxByte;
	char minByteString[20]="no";
	char maxByteString[20]="no";

	// 입력 관련 에러 처리
	if (argc != ARGMAX) { //인자 개수도 못채웠다면
				printf("ERROR: Arguments error\n");
				return;
	}

	char inputPath[PATHMAX];
	//인자로 받은 디렉토리가 옳은가?
	if(argv[4][0]=='~'){ //~dir1을 입력받으면... 이건 에러처리를 해줘야함.
			char* homePath=getenv("HOME");
			if(strlen(argv[4])==1){
				strcpy(inputPath,homePath);
			}
			else if(argv[4][1]=='/'){ // ~이후에 /만 오는게 보장된다면 일단 
					//적어도 경로양식이  틀린건 아님.
				sprintf(inputPath,"%s%s",homePath,argv[4]+1);// /를 포함한 이후.
			}
			else{ //~로 시작하고 길이가 2이상인데 ~이후에 / 가 아닌 경우.
				fprintf(stderr,"error....directoryPath from ~\n");
				return;
			}
	}
	else{
		strcpy(inputPath,argv[4]);
	}

	//받은 디렉토리 경로가 이상하면 다시
	if (realpath(inputPath, dirname) == NULL) { //탐색시작 디렉토리의 절대경로 얻기
				printf("ERROR: Path exist error\n");
				return;
	}
	
	//디렉토리 경로는 이상 없는데 lstat 안열리면 다시.
	if (lstat(dirname, &statbuf) < 0) {
				fprintf(stderr, "lstat error for %s\n", dirname);
				exit(1);
	}
	if (!S_ISDIR(statbuf.st_mode)) {
				printf("ERROR: Path must be directory\n");
				return;
	}

	//확장자에 대해
	if(argv[1][0]=='*'){
		if(strlen(argv[1])==1){
			//오케이
		}
		else if(strlen(argv[1])>2 && argv[1][1]=='.'){
			//오케이
		}
		else
				return; // *. 과 *다른거   이런 경우를 걸러냄.
	}
	else{ //첫글자가 *이 아니면 걸러냄.
		return;
	}

	int errorSize=0;
	//인풋으로 받은 크기가 제대로된 입력인지에 대한 상태. ~이면 1을 리턴함.
	//정수로 시작을 안하면 일단 에러처리.
	int state=isInputByteWave(argv[2]);
	if(state<0){
		printf("[MINSIZE] input error\n");
		return;
	}
	else if(state){
		minByte=0;
		strcpy(minByteString,"zero");
	}
	else{
		//소수점이 두번 들어가거나 문자가 중간에 껴있으면 에러처리.
		//하지만 끝에 두자리에는 kb,mb,gb가 올 수 있음.
		//이 조건들을 만족하면 해당하는 값으로 변환해서 리턴해줌.
		minByte=returnByteSize(argv[2],&errorSize);
		if(errorSize){ //변환과정에서 잘못됨.
			printf("[MINSIZE] input error\n");
			return;	
		}
	}
	
	errorSize=0;
	state=isInputByteWave(argv[3]);
	if(state<0){
		printf("[MAXSIZE] input error\n");
		return;
	}
	else if(state){
		maxByte=0;
		strcpy(maxByteString,"infinite");
	}
	else{
		maxByte=returnByteSize(argv[3],&errorSize);
		if(errorSize){ //변환과정에서 잘못됨.
			printf("[MAXSIZE] input error\n");
			return;	
		}
	}


	//여기까지 왔다는 것은 입력이 정상적이란 것.
	pid_t pid=fork(); //자식프로세스면 0임. 부모프로세스면 자식프로세스값을 리턴받음. 즉,양수
	if(pid>0){ //부모프로세스는 잠시 대기
		wait(NULL);
		return; //헬프 명령어 끝나면 다시 프롬프트 띄우기.
	}
	else if(pid==0){ //다렉토리의  절대경로
		execl("./fsha1",argv[0],argv[1],argv[2],argv[3],minByteString,maxByteString,dirname,(char*)0);
		//자식은 죽음. 대신 새로 생긴 프로세스가 그 프로세스id를 이어받음.
	}
}

void command_help(){
			pid_t pid=fork(); //자식프로세스면 0임. 부모프로세스면 자식프로세스값을 리턴받음. 즉,양수
			if(pid>0){ //부모프로세스는 잠시 대기
				wait(NULL);
				 //헬프 명령어 끝나면 다시 프롬프트 띄우기.
			}
			else if(pid==0){
				execl("./help","help",(char*)0); //자식프로세스쪽에서 다른 프로세스를 실행시킴.
				//자식은 죽음. 대신 새로 생긴 프로세스가 그 프로세스id를 이어받음.
			}
}


// 입력 문자열 토크나이징 함수. string을 구분자를 기준으로 구분해서 argv이차원배열에 저장해줌.
//그리고 구분 개수 리턴
int split(char* string, char* seperator, char* argv[])
{
		int argc = 0;
		char* ptr = NULL;

		ptr = strtok(string, seperator);
		while (ptr != NULL) {
				argv[argc++] = ptr;
				ptr = strtok(NULL, " ");
		}
		return argc;
}
