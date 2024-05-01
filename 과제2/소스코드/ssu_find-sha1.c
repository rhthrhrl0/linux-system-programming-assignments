#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/time.h>
#include<fcntl.h>
#include<string.h>
#include<openssl/sha.h>
#include<time.h>
#include<dirent.h>
#define BUFMAX 1024
#define PATHMAX 4097
#include"customHeader.h"
#include"ssu_queue-sha1.h"

#define BUFSIZE 1024*16
#define ARGMAX 3


void firstFindSQ(char* extension,char* startDirname,char* minByte,char* minByteState,char* maxByte,char* maxByteState);
void findStartSHA1(char* extension,char* startDirname,char* minByte,char* minByteState,char* maxByte,char* maxByteState);
void getSHA1hash(char* pathName,unsigned char* sha1);
void printSearchingTime();
int split(char* string, char* seperator, char* argv[]);
off_t customAtoi(char* input);

char currentPath[PATHMAX]; //이 프로그램이 있는 경로를 기억.
char TrashPath[PATHMAX]; //쓰레기통 경로.
SizeQueue* sq;
Root* root;
struct timeval start_t, end_t;
int main(int argc,char* argv[]){
	
	if(getcwd(currentPath,PATHMAX)==NULL){
		fprintf(stderr, "getcwd fail.....\n");
		exit(1);
	}

	if(realpath("./Trash",TrashPath)==NULL){
		fprintf(stderr,"쓰레기통 디렉토리가 안보입니다.\n");
		exit(1);
	}

	gettimeofday(&start_t,NULL);
	firstFindSQ(argv[1],argv[6],argv[2],argv[4],argv[3],argv[5]);
	//탐색시작 디렉토리,     minByte,        maxByte
	findStartSHA1(argv[1],argv[6],argv[2],argv[4],argv[3],argv[5]);
	cleanroot(root); //혼자 해쉬값을 갖는 목록들은 제거한다.
	sortroot(root); //바이트 크기 순으로 정렬.
	deletequeueSize(sq); //해쉬값의 개수를 세던 큐는 이제 없어도 됨.
	gettimeofday(&end_t,NULL);

	//현재 디렉토리로 돌아오기.
	if(chdir(currentPath)<0){
		fprintf(stderr,"current back directory change error\n");
		exit(1);
	}

	//탐색종료시간 출력해야함.
	if(root->count==0){
		printf("No duplicates in %s\n",argv[6]);
		printSearchingTime();
		printf("\n\n");
		exit(1); //프롬프트로 다시 나감.
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
		if(argcP<=1){
				printStart=0;
				continue;
		}
		
		setC=0;
		indexInSet=0;
		if( (strcmp(argvP[1],"d")==0) && argcP==3 ){
			//잘못된 숫자이면 0을 저장해서 주는 함수필요.
			setC=customAtoi(argvP[0]);
			indexInSet=customAtoi(argvP[2]); //에러면 0을 리턴받음.
			//범위안에 들어가면 함수실행
			if(setC!=0 && indexInSet!=0){
				if(setC <= root->count){
					Queue* q=queueInSet(root,setC); //해당 세트를 가리키는 링크드리스트를 리턴.
					if(indexInSet<=q->count){
						int* deleteList=(int*)malloc(sizeof(int)*(q->count));
						memset((int*)deleteList,0,sizeof(int)*(q->count));
						deleteList[indexInSet-1]=1; //삭제할 인덱스.
						deroot(root,setC,'d',deleteList);
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
		else if(strcmp(argvP[1],"i")==0 && argcP==2 ){
			
			setC=customAtoi(argvP[0]);
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

				deroot(root,setC,'i',deleteList);
				free(deleteList);
			}
			else{
				printStart=0;
				fprintf(stderr,"no match.... do reCommand\n");
				continue;
			}

		}	
		else if(strcmp(argvP[1],"f")==0 && argcP==2){
			setC=customAtoi(argvP[0]);
			if(setC!=0 && setC<=root->count){
				deroot(root,setC,'f',NULL);
			}
			else{
				printStart=0;
				continue;
			}
		}
		else if(strcmp(argvP[1],"t")==0 && argcP==2){
			setC=customAtoi(argvP[0]);
			if(setC!=0 && setC<=root->count){
				deroot(root,setC,'t',NULL);
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
	
	deleteroot(root);
	exit(0);
}

void firstFindSQ(char* extension,char* startDirname,char* minByte,char* minByteState,char* maxByte,char* maxByteState){
	
		off_t minByteNum;
		off_t maxByteNum;
		int isInfinite=0;
		int errorSize=0;
		if(strcmp(minByteState,"zero")==0){
			minByteNum=0;
		}
		else{
			minByteNum=returnByteSize(minByte,&errorSize);
		}

		if(strcmp(maxByteState,"infinite")==0){
			isInfinite=1;
		}
		else{
			maxByteNum=returnByteSize(maxByte,&errorSize);
		}


		sq=(SizeQueue*)malloc(sizeof(SizeQueue));
		initqueueSize(sq);

		//bfs탐색을 위한 큐
		Queue* q=(Queue*)malloc(sizeof(Queue));
		unsigned char sha1[SHA_DIGEST_LENGTH]={0,};
		initqueue(q,0,sha1); //큐초기화. 이건 단순히 디렉토리를 담을 것임.
		char* startPath=startDirname;
		enqueue(q,startPath); //탐색시작경로를 큐에 추가.
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
						strcmp(fileList[i]->d_name,"..")){
						if((lstat(fileList[i]->d_name,&buf)==0)){
								
								if((buf.st_mode & S_IFMT)==S_IFREG){ //정규파일인 경우.
									if (realpath(fileList[i]->d_name, nextPath) == NULL) {
											printf("ERROR: Path exist error-> %s/%s\n",dPath,fileList[i]->d_name);
									}
								
									if(isInfinite){
										if(buf.st_size>=minByteNum){
											if(strcmp(extension,"*")==0){
								
												enqueueSize(sq,buf.st_size);
												
											}
											else{ //지정한 확장자가 있는 경우.
												size_t exLength=strlen(extension);
												size_t nameLength=strlen(fileList[i]->d_name);
												if(nameLength >= exLength-1){
													if(strstr(((fileList[i]->d_name)+nameLength-1-exLength+2),(extension+1))!=NULL){

														
														
														enqueueSize(sq,buf.st_size);
														
													}	
												}
											}
										}
									}
									else{
										if(buf.st_size>=minByteNum && buf.st_size<=maxByteNum){
											if(strcmp(extension,"*")==0){
												
												
												enqueueSize(sq,buf.st_size);
												
											}
											else{ //지정한 확장자가 있는 경우.
												size_t exLength=strlen(extension);
												size_t nameLength=strlen(fileList[i]->d_name);
												if(nameLength >= exLength-1){
													if(strstr(((fileList[i]->d_name)+nameLength-1-exLength+2),(extension+1))!=NULL){

													
														
														enqueueSize(sq,buf.st_size);
														
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
													strcmp(nextPath,TrashPath) ){
											enqueue(q,nextPath);
										}
								}

						}
				}
				free(fileList[i]);
			}
			free(fileList);
		}
		cleanqueueSize(sq);
}


void findStartSHA1(char* extension,char* startDirname,char* minByte,char* minByteState,char* maxByte,char* maxByteState){
		
		off_t minByteNum;
		off_t maxByteNum;
		int isInfinite=0;
		int errorSize=0;
		if(strcmp(minByteState,"zero")==0){
			minByteNum=0;
		}
		else{
			minByteNum=returnByteSize(minByte,&errorSize);
		}

		if(strcmp(maxByteState,"infinite")==0){
			isInfinite=1;
		}
		else{
			maxByteNum=returnByteSize(maxByte,&errorSize);
		}


		root=(Root*)malloc(sizeof(Root));
		initroot(root);
		Queue* q=(Queue*)malloc(sizeof(Queue));
		unsigned char sha1[SHA_DIGEST_LENGTH]={0,};
		initqueue(q,0,sha1); //큐초기화. 이건 단순히 디렉토리를 담을 것임.
		char* startPath=startDirname;
		enqueue(q,startPath); //탐색시작경로를 큐에 추가.
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
						strcmp(fileList[i]->d_name,"..")){
						if((lstat(fileList[i]->d_name,&buf)==0)){
								
								if((buf.st_mode & S_IFMT)==S_IFREG){ //정규파일인 경우.
									if (realpath(fileList[i]->d_name, nextPath) == NULL) {
											printf("ERROR: Path exist error-> %s/%s\n",dPath,fileList[i]->d_name);
									}

									if(isInfinite){
										if(buf.st_size>=minByteNum){
											if(strcmp(extension,"*")==0){
												getSHA1hash(fileList[i]->d_name,sha1);
												if(sha1!=NULL){
													enroot(root,sq,nextPath,buf.st_size,sha1);
												}
											}
											else{ //지정한 확장자가 있는 경우.
												size_t exLength=strlen(extension);
												size_t nameLength=strlen(fileList[i]->d_name);
												if(nameLength >= exLength-1){
													if(strstr(((fileList[i]->d_name)+nameLength-1-exLength+2),(extension+1))!=NULL){

														getSHA1hash(fileList[i]->d_name,sha1);
														if(sha1!=NULL){
															enroot(root,sq,nextPath,buf.st_size,sha1);
														}
													}	
												}
											}
										}
									}
									else{
										if(buf.st_size>=minByteNum && buf.st_size<=maxByteNum){
											if(strcmp(extension,"*")==0){
												getSHA1hash(fileList[i]->d_name,sha1);
												if(sha1!=NULL){
													enroot(root,sq,nextPath,buf.st_size,sha1);
												}
											}
											else{ //지정한 확장자가 있는 경우.
												size_t exLength=strlen(extension);
												size_t nameLength=strlen(fileList[i]->d_name);
												if(nameLength >= exLength-1){
													if(strstr(((fileList[i]->d_name)+nameLength-1-exLength+2),(extension+1))!=NULL){

														getSHA1hash(fileList[i]->d_name,sha1);
														if(sha1!=NULL){
															enroot(root,sq,nextPath,buf.st_size,sha1);
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
													strcmp(nextPath,TrashPath) ){
											enqueue(q,nextPath);
										}
								}

						}
				}
				free(fileList[i]);
			}
			free(fileList);
		}

}

void getSHA1hash(char* pathName,unsigned char* sha){
	int fd;
	if((fd=open(pathName,O_RDONLY))<0){
		fprintf(stderr,"open error..can't find sha1\n");
		sha=NULL;
		return;
	}
 	int readSize=0;
	static unsigned char buf[BUFSIZE];
	SHA_CTX c; //해쉬값을 초기화 해갈것임.
	
	SHA1_Init(&c); //해쉬값 계산 전 초기화
	while((readSize=read(fd,buf,BUFSIZE))>0){
		SHA1_Update(&c,buf,(unsigned long)readSize);
	}
	SHA1_Final(&(sha[0]),&c); //최종적인 해쉬값을 저장. 
	close(fd);
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
