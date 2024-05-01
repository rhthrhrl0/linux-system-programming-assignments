#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <dirent.h>
#include <ctype.h>
#include <math.h>
#include <openssl/md5.h>
#include <pthread.h>
#include<fcntl.h>
#include<dirent.h>
#include<pwd.h>
#define BUFMAX 1024
#define PATHMAX 4097
#define HASHMAX 33
#include "customHeader.h"
#include "ssu_queue-md5.h"
#define BUFSIZE 1024*16
#define STRMAX 10000
#define ARGMAX 11 //수정필요

int tokenize_main(char* input, char* argv[]);
void ssu_help();
int command_fmd5(int argc, char* argv[]);
int isInputByteWave(char* input);
int command_list(int argc,char* argv[]);
int command_restore(int argc,char* argv[]);
int command_trash(int argc,char* argv[]);

void findStartMD5(char* extension,off_t minByte,char* minByteState,off_t maxByte,char* maxByteState);
void getMD5hash(char* pathName, char* hash_result);
void printSearchingTime();
int split(char* string, char* seperator, char* argv[]);
off_t customAtoi(char* input);

void get_path_from_home(char *path, char *path_from_home);

char currentPath[PATHMAX]; //이 프로그램이 있는 현재경로를 기억.
Root* root; //전역 변수이다....
Root* collect;
TrashRoot* trashRoot;
struct timeval start_t, end_t;

extern int opterr;

char same_size_files_dir[PATHMAX]; // 탐색하기 위한 디렉토리.


void get_fullpath(char *target_dir, char *target_file, char *fullpath)
{
		strcat(fullpath, target_dir);

		if(fullpath[strlen(target_dir) - 1] != '/')
				strcat(fullpath, "/");

		strcat(fullpath, target_file);
		fullpath[strlen(fullpath)] = '\0';
}

int get_dirlist(char *target_dir, struct dirent ***namelist)
{
		int cnt = 0;

		if ((cnt = scandir(target_dir, namelist, NULL, alphasort)) == -1){
				printf("ERROR: scandir error for %s\n", target_dir);
				return -1;
		}

		return cnt;
}

void remove_files(char *dir)
{
		struct dirent **namelist;
		int listcnt = get_dirlist(dir,&namelist);

		for (int i = 0; i < listcnt; i++){
				char fullpath[PATHMAX] = {0, };

				if (!strcmp(namelist[i]->d_name, ".") || !strcmp(namelist[i]->d_name, "..")){
						free(namelist[i]);	
						continue;
				}

				get_fullpath(dir, namelist[i]->d_name, fullpath);
				remove(fullpath); //기존 파일 지우기... 
				free(namelist[i]);
		}
		free(namelist);
}

// 홈디렉토리의 아래에 20200000디렉토리를 만든다..... 만약 기존에 있으면 지워버림.
void get_same_size_files_dir(void)
{
		get_path_from_home("~/20182580", same_size_files_dir); 

		if (access(same_size_files_dir, F_OK) == 0) //기존에 있으면 지우기.
					remove_files(same_size_files_dir);
		else
					mkdir(same_size_files_dir, 0755);
}

void write_size_file(off_t size,char* nextPath,char* hash){
		FILE* fp;
		char size_file_name[PATHMAX];
		sprintf(size_file_name,"%s/%ld",same_size_files_dir,size);
		
		if ((fp = fopen(size_file_name, "a")) == NULL){ // a모드는 없으면 새로 만든다. 있으면 기존꺼 뒤에 붙여쓰기. 
				printf("ERROR: fopen error for %s\n", nextPath);
				return;
		}
		char write_content[10000];
		sprintf(write_content,"%s %s\n",hash,nextPath);
		fwrite(write_content,strlen(write_content),1,fp); //써라..
		
		fclose(fp);
}

int pthread_num=1;
typedef struct thread_data{
	int thread_index;
	int numInDir;
	struct dirent** fileList;
}thread_data;

pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;

struct thread_data* t_data;

void* find_for_root(void * arg){

		thread_data* data;
		int thread_index;
		int numInDir;
		struct dirent** fileList;

		data=(thread_data*)arg;
		thread_index=data->thread_index;
		numInDir=data->numInDir;
		fileList=data->fileList;



		Root* thread_root=(Root*)malloc(sizeof(Root));
		initroot(thread_root);
		for(int i=thread_index-1 ; i<numInDir; ){
				if(strcmp(fileList[i]->d_name,"..") && strcmp(fileList[i]->d_name,".")){
				
					FILE* fp;
					if((fp=fopen(fileList[i]->d_name,"r"))==NULL){
						fprintf(stderr,"error-fopen in find_for_root\n");
						i+=pthread_num;
						continue;
					}

					char filepath[PATHMAX];
					char hash[HASHMAX];
					char line[10000];
					off_t filesize=atol(fileList[i]->d_name);


					Root* subroot=(Root*)malloc(sizeof(Root));
					initroot(subroot);
					while(fgets(line, 10000, fp) != NULL){
						strncpy(hash, line, HASHMAX); // 해쉬값 + 공백까지 복사시킨다.
						hash[HASHMAX-1] = '\0'; // 마지막 공백은 널로 채운다.
	
						strcpy(filepath, line+HASHMAX); // 파일의 경로명을 파일의 줄에서 가져온다.
						filepath[strlen(filepath)-1] = '\0'; //fgets로 가져온 개행을 널로 대체.
	
						enroot(subroot,filepath,filesize,hash);
					}
					
					cleanroot(subroot);
					insert_sub_root(thread_root,subroot);
					
					fclose(fp);
				}

				i+=pthread_num;	
		}
		
		pthread_mutex_lock(&mutex);
		insert_sub_root(root,thread_root);
		pthread_mutex_unlock(&mutex);

		pthread_exit(NULL);
}


thread_data thread_data_array[5];
void add_root_size_file(Root* root){

		if(chdir(same_size_files_dir)<0){
				printf("chdir_error\n");
				return;
		}
		
		struct dirent** t_fileList;
	
		int numInDir=0; //디렉토리 안에 파일 혹은 서브 디렉토리 개수를 리턴받을 변수
		if((numInDir=scandir(same_size_files_dir,&t_fileList,NULL,alphasort))==-1){ //파일리스트 불러옴.
				 fprintf(stderr,"error: 디렉토리를 열 수 없습니다.\n");
		}
		

		pthread_t* tid=malloc(sizeof(pthread_t)*pthread_num);
		memset(thread_data_array,0,sizeof(thread_data)*5);

		for(int i=0;i<pthread_num;i++){
			thread_data_array[i].thread_index=i+1;
			thread_data_array[i].numInDir=numInDir;
			thread_data_array[i].fileList=t_fileList;
			pthread_create(&tid[i],NULL,find_for_root,(void*)&thread_data_array[i]);	
		}

		for(int i=0;i<pthread_num;i++){
			pthread_join(tid[i],NULL);
		}

		for(int i=0;i<numInDir;i++){
				free(t_fileList[i]);
		}
		free(t_fileList);
		
		free(tid);		
}

Queue* q;
int fmd5(char* extension,off_t minByte,off_t maxByte,char* minByteString,char* maxByteString,char* dirName){

		if(root!=NULL){
				deleteroot(root);	
		}
		if(collect!=NULL){
				deleteroot(collect);
		}
		if(getcwd(currentPath,PATHMAX)==NULL){
				fprintf(stderr, "getcwd fail.....\n");
				return 1;
		}

		if(q!=NULL)
				free(q);
		q=(Queue*)malloc(sizeof(Queue));
		get_same_size_files_dir(); //탐색하기 위한 홈 아래의 20182580 디렉토리 준비 및 초기화.

		gettimeofday(&start_t,NULL);
		
		char md5[HASHMAX];
		memset(md5,0,HASHMAX);
		initqueue(q,0,md5); //큐초기화. 이건 단순히 디렉토리를 담을 것임.
		enqueue(q,dirName); //탐색시작경로를 큐에 추가.
		
		findStartMD5(extension,minByte,minByteString,maxByte,maxByteString);

		root=(Root*)malloc(sizeof(Root));
		initroot(root);

		collect=(Root*)malloc(sizeof(Root));
		initroot(collect);
	
		add_root_size_file(root);

		cleanroot(root); //혼자 해쉬값을 갖는 목록들은 제거한다.
		sortroot(root); //바이트 크기 순으로 정렬.
		gettimeofday(&end_t,NULL);

		//현재 디렉토리로 돌아오기.
		if(chdir(currentPath)<0){
				fprintf(stderr,"current back directory change error\n");
				return 1;
		}

		//탐색종료시간 출력해야함.
		if(root->count==0){
				printf("No duplicates in %s\n",dirName);
				printSearchingTime();
				printf("\n\n");
				return 0; //프롬프트로 다시 나감.
		}

		int argcP = 0;
		char input[BUFMAX];
		char* argvP[ARGMAX];

		off_t setC=0;
		off_t indexInSet=0;
		int printStart=1;
		int isFirstPrint=1;

		while(1){
				if(printStart){
						cleanroot(root); 
						if(root->count==0){
								printf("No duplicates anymore\n");
								break;
						}
						printroot(root); 
						//출력할거 출력.

						//탐색 시간 출력은 맨처음에만.
						if(isFirstPrint){
								printSearchingTime();
								printf("\n\n");
								isFirstPrint=0;
						}
				}
				printStart=1; //특별한 일 없으면 다음 >>프롬프트때 리스트 출력함.
				printf(">> ");
				fgets(input, sizeof(input), stdin);
				input[strlen(input) - 1] = '\0';
				argcP = split(input," ", argvP);
				
				if( argcP==1 && (strcmp(argvP[0],"exit")==0) ){
						printf(">> Back to Prompt\n");
						break;
				}
				if(argcP<=1){ //delete 밖에 없다면... 다시.
						printStart=0;
						continue;
				}
				if(strcmp(argvP[0],"delete")){ //exit도 아니고 delete도 아니라면...
						printStart=0;
						continue;
				}
				
				int l=0,d=0,i=0,f=0,t=0;
				int op;
				optind=1;

				setC=0;
				indexInSet=0;
				int delete_command_error=0;
				while((op=getopt(argcP,argvP,"l:d:ift"))!=-1){
					
					if(delete_command_error)
							break;

					switch(op){
							case 'l':
									setC=customAtoi(optarg);
									if(setC==0){
										delete_command_error=1;
										break;
									}
									l=1;
									break;
							case 'd':
									indexInSet=customAtoi(optarg);
									if(indexInSet==0){
										delete_command_error=1;
										break;
									}
									d=1;
									break;

							case 'i':
									i=1;
									break;
							case 'f':
									f=1;
									break;
							case 't':
									t=1;
									break;
							case '?':
									delete_command_error=1;
									break;
					}
				}

				if(delete_command_error){
					printf("option error\n");
					printStart=0;
					continue;
				}

				if(l==0){ // l옵션이 꺼져 있다면
					printf("option -l is need...\n");
					printStart=0;
					continue;
				}
				else{ // l옵션은 켜진게 보장됨.
					int option_count=0;
					if(d) option_count++;
					if(i) option_count++;
					if(f) option_count++;
					if(t) option_count++;

					if(option_count!=1){ // d,i,f,t 중 하나만  있어야 함.
						printStart=0;
						continue;
					}
				}

				// d옵션이 켜져있다면
				if(d){ 
						//잘못된 숫자이면 0을 저장해서 주는 함수필요.
						//범위안에 들어가면 함수실행
						if(setC!=0 && indexInSet!=0){
								if(setC <= root->count){
										Queue* q=queueInSet(root,setC); //해당 세트를 가리키는 링크드리스트를 리턴.
										if(indexInSet<=q->count){
												int* deleteList=(int*)malloc(sizeof(int)*(q->count));
												memset((int*)deleteList,0,sizeof(int)*(q->count));
												deleteList[indexInSet-1]=1; //삭제할 인덱스.
												deroot(root,collect,setC,'d',deleteList);
												free(deleteList);
										}
										else{
												printStart=0;
												continue;
										}
								}
								else{
										printStart=0;
										continue;
								}
						}
						else{
								printStart=0;
								fprintf(stderr,"no match... do reCommand\n");
								continue;
						}
				}
				else if(i){ // i옵션이 켜져있는 경우

						if(setC!=0 && setC<=root->count){
								Queue* q=queueInSet(root,setC);
								int* deleteList=(int*)malloc(sizeof(int)*(q->count));
								memset((int*)deleteList,0,sizeof(int)*(q->count));

								int inputError=0;
								Node* n=q->front;
								for(int i=0;i< q->count;i++){
										printf("Delete \"%s\"? [y/n] ",n->path);
										fgets(input, sizeof(input), stdin);
										input[strlen(input) - 1] = '\0';
										argcP = split(input," ", argvP);
										if(argcP==1){
												if(argvP[0][0]=='y' ||
																argvP[0][0]=='Y'){
														deleteList[i]=1;
												}
												else if(argvP[0][0]=='n'||
																argvP[0][0]=='N'){

												}
												else{
														inputError=1;
														break;
												}
										}
										else{ 
												inputError=1;	
												break;
										}
										n=n->next;	
								}

								if(inputError){
										printStart=0;
										free(deleteList);
										continue;
								}

								deroot(root,collect,setC,'i',deleteList);
								free(deleteList);
						}
						else{
								printStart=0;
								fprintf(stderr,"no match.... do reCommand\n");
								continue;
						}

				}	
				else if(f){ // f옵션 켜져있는 경우
						if(setC!=0 && setC<=root->count){
								deroot(root,collect,setC,'f',NULL);
						}
						else{
								printStart=0;
								continue;
						}
				}
				else if(t){ // t옵션 켜져있는 경우
						if(setC!=0 && setC<=root->count){
								deroot(root,collect,setC,'t',NULL);
						}
						else{
								printStart=0;
								continue;
						}
				}
				else{
						printStart=0;
						continue;
				}

				printf("\n\n");	
		}

		return 0;
}

void findStartMD5(char* extension,off_t minByte,char* minByteState,off_t maxByte,char* maxByteState){		
		int isInfinite=0;

		if(strcmp(maxByteState,"infinite")==0){
				isInfinite=1;
		}
		
		Queue* subq=(Queue*)malloc(sizeof(Queue));
		char md5[HASHMAX];
		memset(md5,0,HASHMAX);
		initqueue(subq,0,md5); //큐초기화. 이건 단순히 디렉토리를 담을 것임.

		char nextPath[PATHMAX]; //큐에 넣을 경로를 담을 문자열
		struct stat buf;
		char dPath[PATHMAX];
		struct dirent** fileList=NULL;
		int numInDir;
		while(!isEmpty(q)){ //큐가 빌때까지
				
				dequeue(q,dPath); //큐에 있는 디렉토리절대경로를 하나 빼서 dPath에 저장
		
				numInDir=0; //디렉토리 안에 파일 혹은 서브 디렉토리 개수를 리턴받을 변수
				if((numInDir=scandir(dPath,&fileList,NULL,alphasort))==-1){ //파일리스트 불러옴.
						fprintf(stderr,"error: 디렉토리를 열 수 없습니다.\n");
				}
				if(chdir(dPath)<0){
						fprintf(stderr,"directory change error\n");
				}

				for(int i=0;i<numInDir;i++){
						if(strcmp(fileList[i]->d_name,".") &&
									strcmp(fileList[i]->d_name,"..") &&
											strcmp(fileList[i]->d_name,logFile)){
								if((lstat(fileList[i]->d_name,&buf)==0)){

										if((buf.st_mode & S_IFMT)==S_IFREG){ //정규파일인 경우.
												if (realpath(fileList[i]->d_name, nextPath) == NULL) {
														printf("ERROR: Path exist error-> %s/%s\n",dPath,fileList[i]->d_name);
												}

												if(isInfinite){
														if(buf.st_size>=minByte){
																if(strcmp(extension,"*")==0){
																		memset(md5, 0, HASHMAX);
																		getMD5hash(fileList[i]->d_name,md5);
																		if(md5!=NULL){
																				write_size_file(buf.st_size,nextPath,md5);
																		}
																}
																else{ //지정한 확장자가 있는 경우.
																		size_t exLength=strlen(extension);
																		size_t nameLength=strlen(fileList[i]->d_name);
																		if(nameLength >= exLength-1){
																				if(strstr(((fileList[i]->d_name)+nameLength-1-exLength+2),(extension+1))!=NULL){
																						memset(md5, 0, HASHMAX);
																						getMD5hash(fileList[i]->d_name,md5);
																						if(md5!=NULL){
																							
																								write_size_file(buf.st_size,nextPath,md5);
																						}
																				}	
																		}
																}
														}
												}
												else{
														if(buf.st_size>=minByte && buf.st_size<=maxByte){
																if(strcmp(extension,"*")==0){
																		memset(md5, 0, HASHMAX);
																		getMD5hash(fileList[i]->d_name,md5);
																		if(md5!=NULL){

																				write_size_file(buf.st_size,nextPath,md5);
																		}
																}
																else{ //지정한 확장자가 있는 경우.
																		size_t exLength=strlen(extension);
																		size_t nameLength=strlen(fileList[i]->d_name);
																		if(nameLength >= exLength-1){
																				if(strstr(((fileList[i]->d_name)+nameLength-1-exLength+2),(extension+1))!=NULL){
																						memset(md5, 0, HASHMAX);
																						getMD5hash(fileList[i]->d_name,md5);
																						if(md5!=NULL){
																								
																								write_size_file(buf.st_size,nextPath,md5);
																						}
																				}	
																		}
																}

														}
												}

										}


										else if((buf.st_mode & S_IFMT)==S_IFDIR){
												if (realpath(fileList[i]->d_name, nextPath) == NULL) {
														printf("ERROR: Path exist error-> %s/%s\n",dPath,fileList[i]->d_name);
												}
												if( strcmp(nextPath,"/proc")&&
																strcmp(nextPath,"/run")&&
																strcmp(nextPath,"/sys")&&
																strcmp(nextPath,TrashPath)&&
																strcmp(nextPath,same_size_files_dir)){

														enqueue(subq,nextPath); // 다음 재귀를 위한 큐에 추가.
												}
										}

								}
						}
						free(fileList[i]);
				}
				free(fileList);
		}
		

		if(subq->count!=0){
				char dPath[PATHMAX];
				while(!isEmpty(subq)){
					dequeue(subq,dPath);
					enqueue(q,dPath);
				}
				findStartMD5(extension,minByte,minByteState,maxByte,maxByteState);
		}
}

void getMD5hash(char* pathName, char* hash_result){
		FILE *fp;
		unsigned char hash[MD5_DIGEST_LENGTH];
		unsigned char buffer[SHRT_MAX];
		int bytes = 0;
		MD5_CTX md5;

		if ((fp = fopen(pathName, "rb")) == NULL){
				printf("ERROR: fopen error for %s\n", pathName);
				hash_result=NULL;
				return;
		}

		MD5_Init(&md5);

		while ((bytes = fread(buffer, 1, SHRT_MAX, fp)) != 0)
				MD5_Update(&md5, buffer, bytes);
								
		MD5_Final(hash, &md5);

		for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
				sprintf(hash_result + (i * 2), "%02x", hash[i]);
		
		hash_result[HASHMAX-1] = 0;

		fclose(fp);

		return;
}

void printSearchingTime(){	
		long int sec;
		long int usec; 
		if(end_t.tv_usec < start_t.tv_usec){
				//tv_sec가 커지면 tv_usec이 0이 되는 경우를 고려.
				sec = end_t.tv_sec - start_t.tv_sec - 1;
				usec = end_t.tv_usec - start_t.tv_usec + 1000000;
		}
		else{
				sec = end_t.tv_sec - start_t.tv_sec;
				usec = end_t.tv_usec - start_t.tv_usec;		
		}

		printf("Searching time: %ld:%06ld(sec:usec)\n",sec,usec);
}


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


off_t customAtoi(char* input){

		off_t num=0;
		int length=strlen(input);
		for(int i=0;i<length;i++){
				if(((int)input[i])>=48 && ((int)input[i]) <=57)
						num=num*10+((int)input[i])-48;
				else
						return 0;
		}
		return num;
}

// 쓰레기통으로 보내진 파일을 위한 쓰레기통경로를 지정. 제거된 파일 정보를 관리하는 디렉토리는....
void get_path_from_home(char *path, char *path_from_home)
{
		char path_without_home[PATHMAX] = {0,};
		char *home_path;

		home_path = getenv("HOME");

		if (strlen(path) == 1){ // ~ 하나 뿐이라면, 뒤에 더이상 추가할건 없다... 그냥 getenv로 얻어온 홈의 절대경로만 있으면 됨.
				strncpy(path_from_home, home_path, strlen(home_path));
		}
		else { 
				strncpy(path_without_home, path + 1, strlen(path)-1);
				sprintf(path_from_home, "%s%s", home_path, path_without_home);
		}
}


void get_trash_path()
{
		char trash_dir[PATHMAX];

		get_path_from_home("~/.Trash",trash_dir);
		mkdir(trash_dir,0777); //홈 아래에 .Trash라는 디렉토리 만들기.
		

		if (getuid() == 0){
				get_path_from_home("~/.Trash/files", TrashPath); //해당 디렉토리 만들기.

				//파일이 이미 존재한다면...
				if (access(TrashPath, F_OK) == 0)
						; // 루트권한이라면..기존에 휴지통에 있었다면... 모두 지움.
				else
						mkdir(TrashPath, 0755); 
				
				get_path_from_home("~/.Trash/info",TrashInfoPath);
				mkdir(TrashInfoPath,0755);
				
				get_path_from_home("~/.duplicate_20182580.log",logFile);
				fd_log=open(logFile,O_CREAT|O_WRONLY|O_APPEND,0644);
		}
		else{
				get_path_from_home("~/.Trash/files", TrashPath);
				mkdir(TrashPath,0777);

				get_path_from_home("~/.Trash/info",TrashInfoPath);
				mkdir(TrashInfoPath,0755);

				get_path_from_home("~/.duplicate_20182580.log",logFile);
				fd_log=open(logFile,O_CREAT|O_WRONLY|O_APPEND,0644);
		}
}

int trashFirst=0;

int main(void){
		get_trash_path(); //휴지통 정의하기.//휴지통 정보파일도  //여기서 로그파일도 오픈하기. 없다면 생성.
		opterr=0; //입력잘못받아도 에러안나게
		passwd_p=getpwuid(getuid());
		
		while(1){
				char input[STRMAX];
				char* argv[ARGMAX];

				int argc = 0;

				printf("20182580> ");
				fgets(input,sizeof(input),stdin);
				input[strlen(input)-1]='\0';
				
				argc=tokenize_main(input,argv);
				argv[argc] = (char*)0;


				if(argc==0)
						continue;

				if(!strcmp(argv[0],"exit"))
						break;
				else if(!strcmp(argv[0],"fmd5")){
						if(command_fmd5(argc,argv)){
								printf("fmd5_error.....\n");
						}
						trashFirst=0;
				}
				else if(!strcmp(argv[0],"list")){ //지정한 대로 정렬해주고 출력해야함 
						if(command_list(argc,argv)){
								printf("list_error....\n");
						}
						trashFirst=0;
				}
				else if(!strcmp(argv[0],"trash")){ //쓰레기통의 목록을 출력할때 정렬해서 출력.
						if(command_trash(argc,argv)){
								printf("trash_error....\n");
						}
				}
				else if(!strcmp(argv[0],"restore")){
						if(trashFirst){ //restore명령어 전에 먼저 trash명령어로 쓰레기통 목록을 툴력해야함.
							if(command_restore(argc,argv)){
									printf("restore_error....\n");
							}
						}
						else{
							printf("you must do trash_command first before restore_command\n");
						}
				}
				else{
						ssu_help();
				}

		}

		close(fd_log);
		printf("Prompt End\n");
		exit(0);
}

int tokenize_main(char* input, char* argv[]){
		char* ptr=NULL;
		int argc=0;
		ptr=strtok(input," ");

		while(ptr!=NULL){
				argv[argc++]=ptr;
				ptr=strtok(NULL," ");
		}

		return argc;
}


void ssu_help(){
		printf("Usage:\n");
		printf("  > fmd5 -e [FILE_EXTENSION] -l  [MINSIZE] -h [MAXSIZE] -d [TARGET_DIRECTORY] -t [THREAD_NUM]\n");
		printf("      >> delete -l [SET_INDEX] -d [OPTARG] -i -f -t\n");
		printf("  > trash -c [CATEGORY] -o [ORDER]\n");
		printf("  > restore [RESTORE_INDEX]\n");
		printf("  > help\n");
		printf("  > exit\n\n");
}

extern int optind;

int command_fmd5(int argc, char* argv[]){

		optind=1;

		pthread_num=1;

		char dirname[PATHMAX]; //얻어낸 시작 디렉토리의 절대경로
		struct stat statbuf; 
		off_t minByte;
		off_t maxByte;
		char minByteString[20]="no";
		char maxByteString[20]="no";
		char inputPath[PATHMAX]; //인풋으로 일단 얻은 경로.
		char extension[100];

		// 입력 관련 에러 처리
		if (argc<9 || argc > 11) { //인자 개수에 문제가 있는 경우.
				printf("ERROR: Arguments error\n");
				return 1;
		}

		int errorSize=0;
		int state;

		//인풋에 대한 에러처리...
		int e,l,h,d;
		e=0;l=0;h=0;d=0;
		int c=0;
		while((c=getopt(argc,argv,"e:l:h:d:t:"))!=-1){
				switch(c){
						case 'e':
								e=1;

								//확장자에 대해
								if(optarg[0]=='*'){
										if(strlen(optarg)==1){
												//오케이
										}
										else if(strlen(optarg)>=2 && optarg[1]=='.'){
												//오케이
										}
										else
												return 1; // * 혹은 *.(확장자) 와 다른거 경우를 걸러냄.
								}
								else{ //첫글자가 *이 아니면 걸러냄.
										return 1;
								}
								strcpy(extension,optarg);
								break;
						case 'l':
								l=1;

								errorSize=0;
								//인풋으로 받은 크기가 제대로된 입력인지에 대한 상태. ~이면 1을 리턴함.
								//정수로 시작을 안하면 일단 에러처리.
								state=isInputByteWave(optarg);
								if(state<0){
										printf("[MINSIZE] input error\n");
										return 1;
								}
								else if(state){
										minByte=0;
										strcpy(minByteString,"zero");
								}
								else{
										//소수점이 두번 들어가거나 문자가 중간에 껴있으면 에러처리.
										//하지만 끝에 두자리에는 kb,mb,gb가 올 수 있음.
										//이 조건들을 만족하면 해당하는 값으로 변환해서 리턴해줌.
										minByte=returnByteSize(optarg,&errorSize);
										if(errorSize){ //변환과정에서 잘못됨.
												printf("[MINSIZE] input error\n");
												return 1;	
										}
								}
								break;
						case 'h':
								h=1;

								errorSize=0;
								state=isInputByteWave(optarg);
								if(state<0){
										printf("[MAXSIZE] input error\n");
										return 1;
								}
								else if(state){
										maxByte=0;
										strcpy(maxByteString,"infinite");
								}
								else{
										maxByte=returnByteSize(optarg,&errorSize);
										if(errorSize){ //변환과정에서 잘못됨.
												printf("[MAXSIZE] input error\n");
												return 1;	
										}
								}
								break;
						case 'd':
								d=1;

								if(optarg[0]=='~'){ //~dir1을 입력받으면... 이건 에러처리를 해줘야함.
										char* homePath=getenv("HOME");
										if(strlen(optarg)==1){
												strcpy(inputPath,homePath);
										}
										else if(optarg[1]=='/'){ // ~이후에 /만 오는게 보장된다면 일단 
												//적어도 경로양식이  틀린건 아님.
												sprintf(inputPath,"%s%s",homePath,optarg+1);// /를 포함한 이후.
										}
										else{ //~로 시작하고 길이가 2이상인데 ~이후에 / 가 아닌 경우.
												fprintf(stderr,"error....directoryPath from ~\n");
												return 1;
										}
								}
								else{
										strcpy(inputPath,optarg);
								}

								//받은 디렉토리 경로가 이상하면 다시
								if (realpath(inputPath, dirname) == NULL) { //탐색시작 디렉토리의 절대경로 얻기
										printf("ERROR: Path exist error\n");
										return 1;
								}

								//디렉토리 경로는 이상 없는데 lstat 안열리면 다시.
								if (lstat(dirname, &statbuf) < 0) {
										fprintf(stderr, "lstat error for %s\n", dirname);
										return 1;
								}
								if (!S_ISDIR(statbuf.st_mode)) {
										printf("ERROR: Path must be directory\n");
										return 1;
								}

								break;
						case 't':
								pthread_num=atoi(optarg);
								if( pthread_num > 5 || pthread_num < 1 ){
										printf("thread num over\n");
										return 1;
								}
								break;
						case '?':
								printf("input error....\n");
								return 1; // 판별  함수 종료시킴.
				}
		}

		if(e && l && h && d){
				//최소 네개는 다 채워져야 문제 없는거임....
		}
		else{
				printf("input error....\n");
				return 1;
		}


		if(strcmp(maxByteString,"infinite") && maxByte<minByte){
				printf("max is small than min\n");
				return 1;
		}

		return fmd5(extension,minByte,maxByte,minByteString,maxByteString,dirname);
}

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

int command_list(int argc,char* argv[]){

		if(root==NULL){
				printf("Please run the fmd5 command first\n");
				return 1;
		}
		if(root->count==0){
				printf("There is no duplicate list... please another fmd5 command first\n");
				return 1;
		}
		optind=1;

		int l=0,c=0,o=0;
		int op;

		int listType;
		char category[100];
		int order;
		//defalut
		listType=0;//파일세트단위가 기본
		strcpy(category,"size"); //크기별이 기본
		order=1; // 오름차순이 기본
		while((op=getopt(argc,argv,"l:c:o:"))!=-1){
				switch(op){
						case 'l':
								if(!strcmp(optarg,"fileset")){
										listType=0;
								}
								else if(!strcmp(optarg,"filelist")){
										listType=1;
								}
								else{
										printf("listType is wrong....\n");
										return 1;
								}
								break;
						case 'c':	
								if(!strcmp(optarg,"filename"))
										strcpy(category,"filename");

								else if(!strcmp(optarg,"size"))
										strcpy(category,"size");

								else if(!strcmp(optarg,"uid"))
										strcpy(category,"uid");

								else if(!strcmp(optarg,"gid"))
										strcpy(category,"gid");

								else if(!strcmp(optarg,"mode"))
										strcpy(category,"mode");

								else{
										printf("category is wrong....\n");
										return 1;
								}
								break;

						case 'o':
								if(!strcmp(optarg,"1"))
										order=1;
								else if(!strcmp(optarg,"-1"))
										order=-1;
								else{
										printf("order is wrong...\n");
										return 1;
								}
								break;
						case '?':
								printf("list_option_unknown_error\n");
								return 1;
								break;
				}
		}

		list_root(root,listType,category,order);
		printroot(root);
}


int command_restore(int argc,char* argv[]){

		if(argc != 2){
				printf("argc have to be only two\n");
				return 1;
		}
	
		int restoreNum=atoi(argv[1]);
		
		if(restoreNum > trashRoot->count || restoreNum < 1){
				printf("%d index is over....\n",restoreNum);
				return 1;
		}

		TrashNode* beforeN=NULL;
		TrashNode* cur=trashRoot->front; //1번 을 가리키고 있음.
		for(int i=1;i<restoreNum;i++){
				beforeN=cur;
				cur=cur->next; 
		}

		if(beforeN==NULL){
			trashRoot->front=cur->next;
		}
		else{
			beforeN->next=cur->next;
		}
		trashRoot->count --;

		//이걸 마치면 복구하길 원하는 정보의 TrashNode를 가리키고 있다.
		char restorePath[PATHMAX];

		strcpy(restorePath,cur->originPath); //원본경로....
		char* tmp=strrchr(restorePath,'/'); //복구해야 할 원본 경로에서 뒤에서 마직막 /를 찾옴.

		char originFileName[PATHMAX]; //원래의 파일이름...
		strcpy(originFileName,tmp+1); // 순수한 파일의 본래 이름만
		char originDir[PATHMAX];
		
		int i;
		for(i=0;i<strlen(restorePath);i++){
			if(tmp==&restorePath[i]){ // 즉, exceptExt에는 / 이전까지의 파일이름만 남는다.
				originDir[i]='\0';
				break;
			}
			else{
				originDir[i]=restorePath[i]; //복구해야할 디렉토리의 경로를 originDir에 저장.
			}
		}
		if(i==0){
			strcpy(originDir,"/"); // i가 0인데 나왔다는 것은 본래 루트에 있었다는 의미다.
		}

		int j=0;
		i++; // /이후의 진짜 파일명만 가져옴...
		for(;i<strlen(restorePath);i++){   
			originFileName[j++]=restorePath[i]; 
		}
		//originFileName 뒤에 널이 있나?

		if(access(originDir,F_OK)){
			printf("The directory %s where this file was located no longer exists\n",originDir);
			return 1;
		}


		struct dirent** fileList=NULL;
		int numInDir;
		if((numInDir=scandir(originDir,&fileList,NULL,alphasort))==-1){ // 쓰레기
				fprintf(stderr,"error: 디렉토리를 열 수 없습니다.\n");
				return 1;
		}
	
		char* ext=strrchr(originFileName,'.'); //ext는 원래 파일명 중 확장자가 있는지 검사.
		char originExtension[PATHMAX];

		char exceptExt[4097]; //마지막 .의 앞 내용
		if(ext!=NULL){ // 확장자가 있었다면....
			strcpy(originExtension,ext);
			for(i=0;i<strlen(originFileName);i++){
					if(ext==&originFileName[i]){ // 즉, exceptExt에는 . 이전까지의 파일이름만 남는다.
							exceptExt[i]='\0';
							break;
					}
					else{
							exceptExt[i]=originFileName[i];
					}
			}
		}
		else{
			strcpy(exceptExt,originFileName);
		}
		//이제 exceptExt에는 . 앞의 디렉토리를 뺀 순수 파일이름만, ext에는 확장자 .포함..... 만약 .이 없으면 널임.

		int num=0;
		for(i=0;i<numInDir;i++){
			if(!strcmp(originFileName,fileList[i]->d_name)){
				num++;
				if(ext!=NULL){
					sprintf(originFileName,"%s.%d%s",exceptExt,num,originExtension); //이름바꾸고
				}
				else{	
					sprintf(originFileName,"%s.%d",exceptExt,num); //이름바꾸고
				}
				i=-1; //다시 i는 0부터 처음부터 비교.
				continue;
			}
		}

		if(i>=numInDir){
			if(!strcmp(originDir,"/")){
					sprintf(restorePath,"%s%s",originDir,originFileName);
			}
			else{
					sprintf(restorePath,"%s/%s",originDir,originFileName);
			}
		}

		time_t restore_time=time(NULL);
		struct tm* restore_t=localtime(&restore_time);
		 
		if(link(cur->trashPath,restorePath)<0) //복구
				printf("can't move in Trash....\n");
		unlink(cur->trashPath); //쓰레기통에서는 삭제....
	

		char hash[HASHMAX];
		getMD5hash(restorePath,hash);
		if(hash!=NULL && root!=NULL && collect!=NULL ){
			restore_enroot(root,collect,restorePath,hash);
		}
		char InfoPath[PATHMAX]; 
		char* infoTmp=strrchr(cur->trashPath,'/');
		sprintf(InfoPath,"%s%s%s",TrashInfoPath,infoTmp,"info");
		unlink(InfoPath);
		//정보 쓰레기 파일도 삭제....

		char log[10000];
		sprintf(log,"[RESTORE] %s %d-%d-%d %d:%d:%d %s\n",cur->originPath,
						restore_t->tm_year+1900, restore_t->tm_mon+1, restore_t->tm_mday,
						restore_t->tm_hour, restore_t->tm_min, restore_t->tm_sec,
						    passwd_p->pw_name);
		 
		write(fd_log,log,strlen(log));

		for(i=0;i<numInDir;i++){
			free(fileList[i]);
		}
		free(fileList);

		//다시 파일 탐색.... 그말은 .. 탐색을 할때 시작디렉토리 및 나머지 변수들이 전역으로 존재해야한다...
		if(trashRoot->count != 0 )
			print_trash(trashRoot);
	
		free(cur->originPath);
		free(cur->trashPath);
		free(cur);
		return 0;
}

int command_trash(int argc,char* argv[]){
	
		optind=1;

		int c=0,o=0;
		int op;

		char category[100];
		int order;
		
		strcpy(category,"filename"); //파일이름이 기본
		order=1; // 오름차순이 기본
		while((op=getopt(argc,argv,"c:o:"))!=-1){
				switch(op){
						case 'c':	
								if(!strcmp(optarg,"filename"))
										strcpy(category,"filename");

								else if(!strcmp(optarg,"size"))
										strcpy(category,"size");

								else if(!strcmp(optarg,"date"))
										strcpy(category,"date");

								else if(!strcmp(optarg,"time"))
										strcpy(category,"time");

								else{
										printf("category is wrong....\n");
										return 1;
								}
								break;

						case 'o':
								if(!strcmp(optarg,"1"))
										order=1;
								else if(!strcmp(optarg,"-1"))
										order=-1;
								else{
										printf("order is wrong...\n");
										return 1;
								}
								break;
						case '?':
								printf("list_option_unknown_error\n");
								return 1;
								break;
				}
		}


		if(trashRoot!=NULL){
				
			TrashNode* deleteNode=trashRoot->front; 
			while(deleteNode!=NULL){
				free(deleteNode->originPath);
				free(deleteNode->trashPath);
				TrashNode* tm=deleteNode;
				deleteNode=deleteNode->next; //다음노드로 이동
				free(tm);
			}
			free(trashRoot);
		}

		trashRoot=(TrashRoot*)malloc(sizeof(TrashRoot));
		trashRoot->count=0;
		trashRoot->front=NULL;
		
		make_trash(trashRoot); //트래쉬 링크드리스트 만들기...
		sort_trash(trashRoot,category,order);
		print_trash(trashRoot);

		trashFirst=1;
		return 0;
}
