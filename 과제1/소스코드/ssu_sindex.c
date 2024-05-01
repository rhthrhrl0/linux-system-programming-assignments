#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<dirent.h>
#include<string.h>
#include<ctype.h>
#include<sys/stat.h>
#include<sys/time.h>
#include<unistd.h>
#include<time.h>

#define BUFFER_SIZE 1024

typedef struct sameFileInfo{
	char path[4097];
	struct stat statInfo;
}sameFileInfo;

int prompt();
int commandFind(char* filePath,char* findStartPath);
void commandExit();
void commandHelp();
long int DFS(char* findPath);
void findNameFunc(char* realPath,char* name); 
void listPrint();
int indexCommand();
int indexPrompt();
int fileCompare(char* compareA,char* compareB);
int directoryCompare(char* compareA,char* compareB);
		
int isDirectoryCompare;

typedef struct Paragraph{
	int startLine;
	int endLine;
}Paragraph;

typedef struct Node{
	char path[4097];
	struct Node* next; //자기자신을 아직 컴파일러는 Node라는 이름으로 모름..
}Node;

typedef struct Queue{
	Node* front; 
	Node* rear;
	int count; //큐의 노드 개수
}Queue;

void initqueue(Queue* q){
	q->front=NULL;
	q->rear=NULL;
	q->count=0;
}

void commonPartRemove(char* a,char* b,char* cprA,char* cprB){
	
	int samePart=0;
	int i=0;
	while((a[i]==b[i]) && (a[i]!='\0') && (b[i]!='\0') ){  
		//둘이 달라지는 시점의i에 빠져나오게 됨.
		
		if(a[i]=='/'){
			samePart=i; // 어디/까지 같은 부분이였는지를 기억
		}
		i++;
	}

	char p[4097];
	i=0;
	for(int j=(samePart+1); j<(int)strlen(a) ;j++,i++){
		p[i]=a[j];
	}
	p[i]='\0';
	strcpy(cprA,p);

	i=0;
	for(int j=(samePart+1); j<(int)strlen(b) ;j++,i++){
		p[i]=b[j];
	}
	p[i]='\0';
	strcpy(cprB,p);
}

void enqueue(Queue* q,char* path){
	Node* newNode=malloc(sizeof(Node));
	strcpy(newNode->path,path);
	newNode->next=NULL; 
	if(q->count==0){ //기존에 큐에 아무것도 없었다면
		q->front = newNode;
		q->rear = newNode;
	}
	else{ //아니라면 기존에 이미 rear가 있었다면
		q->rear->next=newNode; //꼬리의 뒤에 이어주기
		q->rear=newNode; //rear 이동
	}

	q->count=(q->count)+1;	//큐에 노드가 하나 추가
}
void dequeue(Queue* q,char* returnPath){ 
	if(q->count==0) 
		returnPath=NULL; //큐가 비었으면 돌려줄게 없음
	else{
		strcpy(returnPath,q->front->path);
		Node* n=q->front; //임시로 현재의 front를 받아줄 놈
		q->front=q->front->next; //front는 다음 노드로 넘어감
		free(n); //프리해서 해제
		q->count=(q->count)-1;
	}
}

void deletequeue(Queue* q){
	while(q->count!=0){
		Node* n=q->front; //임시로 현재의 front를 받아줄 놈
		q->front=q->front->next; //front는 다음 노드로 넘어감
		free(n); //프리해서 해제
		q->count=(q->count)-1;
	}
}

int countQueue(Queue* q){
	return q->count;
}

int isEmpty(Queue* q){
	return q->count==0?1:0;
}


//기본적으로는 nextPath=a+'/'+b의 값을 갖게 됨.
void nextPathPlus(char* nextPath,char* a, char* b){
	char* ch;
	if(strcmp(a,"/")==0) { //a의 탐색절대경로가 루트인경우는 /를 붙이면 안됨. 
		ch=malloc(sizeof(char)*(strlen(a)+strlen(b)+1)); 
		strcpy(ch,"/");
		strcat(ch,b);	
		strcpy(nextPath,ch);
		free(ch);
	}
	else{
		ch=malloc(sizeof(char)*(strlen(a)+strlen(b)+2)); // 가운데에 '/'들어가야함
		strcpy(ch,a);
		strcat(ch,"/");
		strcat(ch,b);
		strcpy(nextPath,ch);
		free(ch);
	}
}
struct timeval start_t, end_t;
int main(){
	gettimeofday(&start_t,NULL);
	int reStart=1;
	while(reStart){
		reStart=prompt();
	}
	
	exit(0);
}


int prompt(){
	
	char innerCommand[1024];

	printf("20182580>> ");
	fgets(innerCommand,sizeof(innerCommand),stdin); //표준입력으로 해당 크기만큼 받겠다는 뜻
	innerCommand[strlen(innerCommand)-1]='\0'; // fgets()로 받아서 생긴  엔터키의 줄바꿈 문자 제거
	
	char* commandType=strtok(innerCommand," "); 
	
	if(commandType==NULL){
		free(commandType);
		return 1; //이함수 종료하고 프롬프트 재시작	
	}

	else if(strcmp(commandType,"find")==0){
		commandType=strtok(NULL," ");
		if(commandType==NULL){
				free(commandType);
				return 1; //프롬프트 재시작
		}
		char* findFileName=malloc(sizeof(char)*(strlen(commandType)+1));
		strcpy(findFileName,commandType);//파일이름 추출
		
		commandType=strtok(NULL," ");
		if(commandType==NULL){
				free(commandType);
				return 1; //프롬프트 재시작
		}
		char findStartPath[4097];
		strcpy(findStartPath,commandType);//파일경로 추출

		return commandFind(findFileName,findStartPath);		
	}
	else if(strcmp(commandType,"exit")==0){
		commandExit();
	}
	else{
		commandHelp();
		return 1;
	}
}

long int findSize=0; //찾으려는 크기
char findName[257]; //경로를 제외한 단순 파일 혹은 디렉토리 이름
sameFileInfo sameNameSizeList[1024];// 파일리스트 수집
int listIndex=1; //0번 인덱스는 정해짐. 
char start[4097]; 
int commandFind(char* filePath,char* findStartPath){
	listIndex=1; //초기화
	findSize=0;
	Queue* q=malloc(sizeof(Queue)); //큐생성
	initqueue(q);
	char fileRealPath[4097]; //파일의 절대경로
	if(realpath(filePath,fileRealPath)==NULL){ //검색하는 파일을 절대경로로 변환하고  존재안하면
		return 1; //프롬프트 재시작
	}
	strcpy(start,fileRealPath);	
		
	char startPath[4097]; //탐색 시작 절대경로
	if(realpath(findStartPath,startPath)==NULL){
		return 1; //프롬프트 재시작
	}


	struct stat findBuf;
	lstat(fileRealPath,&findBuf); //절대경로로 해당 파일의 정보를 얻어옴.
	if((findBuf.st_mode & S_IFMT) == S_IFREG){
		//찾고자하는건 일반파일임.
		isDirectoryCompare=0;
		findSize=findBuf.st_size; //찾으려는 파일크기 설정	
		findNameFunc(fileRealPath,findName);//절대경로를 토대로 파일의 이름을 알아냄
		
		strcpy(sameNameSizeList[0].path,fileRealPath);
		sameNameSizeList[0].statInfo=findBuf;
		
		initqueue(q); //큐초기화
		enqueue(q,startPath); //탐색시작경로를 큐에 추가.
		char nextPath[4097]; //큐에 넣을 경로를 담을 문자열
		struct stat buf;
		char dPath[4097];
		struct dirent** fileList=NULL;
		int numInDir;
		while(!isEmpty(q)){ //큐가 빌때까지
			dequeue(q,dPath); //큐에 있는 디렉토리절대경로를 하나 빼서 dPath에 저장
			numInDir=0; //디렉토리 안에 파일 혹은 서브 디렉토리 개수를 리턴받을 변수
			numInDir=scandir(dPath,&fileList,NULL,alphasort); //파일리스트 불러옴
			
			for(int i=0;i<numInDir;i++){
				if(strcmp(fileList[i]->d_name,".") &&
						strcmp(fileList[i]->d_name,"..")){
					//디렉토리절대경로와 파일이름을 이어 붙여서 nextPath에 저장하기.
					nextPathPlus(nextPath,dPath, fileList[i]->d_name); 
					
					if((lstat(nextPath,&buf)==0)){
						if(((buf.st_mode & S_IFMT)==S_IFREG) && 
								(strcmp(findName,fileList[i]->d_name)==0) &&
									(buf.st_size==findSize) &&
										(strcmp(fileRealPath,nextPath)!=0)){ //찾는파일경로면 넘어감.			
					
							//정규파일인데 이름이 같고 사이즈가 같으면 
							strcpy(sameNameSizeList[listIndex].path,nextPath);	
							sameNameSizeList[listIndex].statInfo=buf;
							listIndex++;
						}
						else if((buf.st_mode & S_IFMT)==S_IFDIR){ //디렉토리인 경우			
						//	printf("큐에 들어감: %s , 큐 개수: %d \n",nextPath,countQueue(q));
							enqueue(q,nextPath); //디렉토리면 큐에 넣음
							
						}

						free(fileList[i]);
					}
					else {
						
					}
					
				}
				else{
					free(fileList[i]);
				}
			}
			free(fileList);
		} 
	
		deletequeue(q); //큐 삭제
		free(q);

		if(listIndex==1) {//찾으려는 파일과 같은게 없는 경우
			listPrint();
			printf("None\n");
			return 1; 
		}
		else{
			//출력
			listPrint();
			//절대경로 순으로 정렬. 짧은거 먼저.
			return indexPrompt(); //인덱스프롬프트 띄우기로 실행흐름 넘김.
		}
	}
	else if((findBuf.st_mode & S_IFMT) == S_IFDIR){ //찾고자하는게 일반 디렉토리인 경우
		isDirectoryCompare=1;
		findNameFunc(fileRealPath,findName);
		findSize=DFS(fileRealPath); //찾으려는 디렉토리 크기 설정 DFS함수 쓰면 됨.	
		
		strcpy(sameNameSizeList[0].path,fileRealPath);
		sameNameSizeList[0].statInfo=findBuf; //찾으려는 디렉토리의 stat구조체 정보를 저장
		sameNameSizeList[0].statInfo.st_size=findSize;

		initqueue(q); //큐초기화
		enqueue(q,startPath); //탐색시작경로를 큐에 추가.
		char nextPath[4097]; //큐에 넣을 경로를 담을 문자열
		struct stat buf;
		char dPath[4097];
		struct dirent** fileList=NULL;
		int numInDir; 
		//혹시 만약에 넣는 경로가 포함이라면
		while(!isEmpty(q)){ //큐가 빌때까지
			dequeue(q,dPath); //큐에 있는 디렉토리절대경로를 하나 빼서 dPath에 저장
			numInDir=0; //디렉토리 안에 파일 혹은 서브 디렉토리 개수를 리턴받을 변수
			numInDir=scandir(dPath,&fileList,NULL,alphasort); //파일리스트 불러옴
			
			for(int i=0;i<numInDir;i++){
				if(strcmp(fileList[i]->d_name,".") &&
						strcmp(fileList[i]->d_name,"..")){

					nextPathPlus(nextPath,dPath, fileList[i]->d_name); 
		
					if((lstat(nextPath,&buf)==0)){
					
						if((buf.st_mode & S_IFMT)==S_IFDIR){ //디렉토리인 경우			
							enqueue(q,nextPath); //디렉토리면 일단 큐에 넣음
							if((strcmp(findName, fileList[i]->d_name)==0) && (strcmp(fileRealPath,nextPath)!=0)){ 
								//찾는 디렉토리와 해당디렉토리 이름이 같으면
								
								long int size=DFS(nextPath); //여기서 크기 구하기
								if(findSize==size){
									strcpy(sameNameSizeList[listIndex].path,nextPath);	
									sameNameSizeList[listIndex].statInfo=buf;
									sameNameSizeList[listIndex].statInfo.st_size=size;
									listIndex++;
								}
							}	
						}
						free(fileList[i]);
					}
					else {
						
					}
					
				}
				else{
					free(fileList[i]);
				}
			}
			free(fileList);
		} 
	
		deletequeue(q); //큐 삭제
		free(q);

		if(listIndex==1) {//찾으려는 디렉토리와 같은게 없는 경우
			listPrint();
			printf("None\n");
			return 1; 
		}
		else{
			listPrint();
			return indexPrompt(); //인덱스프롬프트를 띄워서 실행흐름을 넘겨줌.
		}
	}
	else{ //파일도 디렉토리도 아닌 경우
		return 1;	
	}
}

void findNameFunc(char* realPath,char* name){
	int FinalSlashIndex=0;
	for(int i=0;i<strlen(realPath);i++){
		if(realPath[i]=='/'){
			FinalSlashIndex=i;
		}
	}
	FinalSlashIndex++;
	for(int i=0;realPath[i]!='\0';i++){
		name[i]=realPath[FinalSlashIndex++];
	}
	name[FinalSlashIndex]='\0';
}


long int DFS(char* findPath){
	
	int fileNumInDirectory=0;
	int* name_count=&fileNumInDirectory;
	
	struct dirent** namelist=NULL;

	if(strcmp(findPath,""))
		*name_count=scandir(findPath,&namelist,NULL,alphasort);
	else //루트부터 탐색시작하는 경우
		*name_count=scandir("/",&namelist,NULL,alphasort);
	
	if(*name_count==0){ //해당 디렉토리에는 파일이나 디렉토리가 없으므로 패스.
		return 0;
	}	
	
	struct stat buf;
	char nextPath[4097];
	long int directoryAllSize=0;
	for(int i=0;i<*name_count;i++){
		if(strcmp(namelist[i]->d_name,".")&&strcmp(namelist[i]->d_name,"..")){
			nextPathPlus(nextPath,findPath, namelist[i]->d_name);
	
			if((lstat(nextPath,&buf)==0)){
				if((buf.st_mode & S_IFMT)==S_IFREG){ //정규파일이면
					directoryAllSize += buf.st_size;
				}
				else if((buf.st_mode & S_IFMT)==S_IFDIR){ //디렉토리인 경우
					char subDirectoryPath[4097];
					nextPathPlus(subDirectoryPath,findPath,namelist[i]->d_name);
					long int subSumSize = DFS(subDirectoryPath);
					directoryAllSize += subSumSize;
				}
				else{	
					directoryAllSize += buf.st_size;
				}
			}
		}	

		free(namelist[i]);
		
	}

	free(namelist);
	
	return directoryAllSize; //종료. 해당 경로에 뭔가 들어있기는 한데 전부 파일인 경우 
}


int indexPrompt(){
	int reIndexPrompt=1;
	while(reIndexPrompt){
		reIndexPrompt=indexCommand();
	}
	return 1; //차이점 출력하고 다시 프롬프트 재시작하도록.
}

int option[127]={0}; //아스키코드를 사용해서 저장할거임. 즉, 옵션i는 option['i'==105]=1로 저장됨.
int indexCommand(){
	memset(option,0,sizeof(int)*127); //옵션 초기화
	char innerCommand[1024];
	
	printf(">> ");
	fgets(innerCommand,sizeof(innerCommand),stdin); //표준입력으로 해당 크기만큼 받겠다는 뜻
	innerCommand[strlen(innerCommand)-1]='\0'; // fgets()로 받아서 생긴  엔터키의 줄바꿈 문자 제거
	
	char* commandType=strtok(innerCommand," "); 
	
	if(commandType==NULL){
		free(commandType);
		return 1; //현재 인덱스프롬프트 종료하고 인덱스프롬프트 재시작	
	}	

	int compareIndex=atoi(commandType); //입력받은 문자열 숫자를 정수로 변환
	if( (compareIndex>=1) && (compareIndex<=listIndex) ){
		commandType=strtok(NULL," ");
		if(commandType==NULL){ //옵션없이 단순파일 혹은 디렉토리 비교 
				free(commandType);

				if((sameNameSizeList[0].statInfo.st_mode & S_IFMT)==S_IFREG){
					//파일비교인 경우
					fileCompare(sameNameSizeList[0].path, sameNameSizeList[compareIndex].path);
				}
				else{
					//디렉토리면 디렉토리비교함수로 흐름넘기기
					directoryCompare(sameNameSizeList[0].path, sameNameSizeList[compareIndex].path);
				}
				return 0; //파일 다른내용 출력하고 인덱스프롬프트는 종료시키고 프롬프트 
		}
		else{
			char optionChar;
			if(strlen(commandType)!=1)
					return 1; //옵션의 길이가 1이 아니면 옵션을 붙어서 입력했을수도 있음. 인프 재시작
			optionChar=commandType[0];
			if(optionChar=='q'|| optionChar=='s' || optionChar=='i' || optionChar=='r')
					option[optionChar]=1; //옵션입력이 들어오면 해당 아스키값  인덱스에 1 저장.
			commandType=strtok(NULL," "); //다음 입력 받음
			while(commandType!=NULL){ //더이상 입력받은 옵션 없을 때까지
				if(strlen(commandType)!=1)
						return 1; //1이 아니면 옵션을 붙어서 입력했을수도 있음. 인프 재시작
				optionChar=commandType[0];
				if(optionChar=='q'|| optionChar=='s' || optionChar=='i' || optionChar=='r'){
						if(option[optionChar]==1)
								return 1; //이미 입력받았던 옵션이면 인프 재시작
						option[optionChar]=1; //옵션입력이 들어오면 해당 아스키값  인덱스에 1 저장.
				}
				commandType=strtok(NULL," ");
			}

			if((sameNameSizeList[0].statInfo.st_mode & S_IFMT)==S_IFREG){//파일비교인 경우
                    fileCompare(sameNameSizeList[0].path,sameNameSizeList[compareIndex].path);
			}
		    else{
					directoryCompare(sameNameSizeList[0].path,sameNameSizeList[compareIndex].path);
			}

			
			return 0; //인덱스프롬프트 종료하고 프롬프트 띄우기
		}


	}
	else{ 
		//atoi는 변환을 실패하면 0을 리턴함. 즉, 1이상이 아닌 인덱스 입력 이쪽 실행흐름으로 넘어옴.
		return 1; //없는 인덱스를 입력했거나 인덱스없이 다른내용이 들어온 경우
	}
		
}



char AfileLine[1025][1025]; //최대 1024줄임.
char BfileLine[1025][1025]; 
int aLine, bLine;
int checkAsame[1025]={0}; //LCS돌면서 찾고 체크
int checkBsame[1025]={0}; //LCS돌면서 찾고 체크
int	paragraphNum=0;
Paragraph* Aparagraph; //찾은 것들을 정리
Paragraph* Bparagraph; //찾은 것들을 정리


void initLCS(){
	
	memset(checkAsame,0,sizeof(int)*1025);
	memset(checkBsame,0,sizeof(int)*1025);

	paragraphNum=0;
	Aparagraph=(Paragraph*)malloc(sizeof(Paragraph)*(aLine+2)); 
	//문단최대 개수는 암만 많아도 줄을  못넘음. 우리가 저장할 문단 개수는 최대: 줄수 + 위아래 허공 문단 한개씩
	Bparagraph=(Paragraph*)malloc(sizeof(Paragraph)*(bLine+2));
	
	Aparagraph[0].startLine=0;
	Aparagraph[0].endLine=0;
	
	Bparagraph[0].startLine=0;
	Bparagraph[0].endLine=0;
	
}

int startLCS(){
	int isDifferent=0;
	int** LCS=malloc(sizeof(int*)*1025);

	for(int i=0;i<1025;i++){
		LCS[i]=malloc(sizeof(int)*1025);		
		for(int j=0;j<1025;j++)
				LCS[i][j]=0;
	}

	if(option['i']){ //옵션i가 켜진경우
		for(int a=1;a<=aLine;a++) //최장 공통 부분 문자라인 개수 구하기
			for(int b=1;b<=bLine;b++){
				if(strcasecmp(AfileLine[a],BfileLine[b])==0) //옵셥i켜져있으면 파일비교 대소관계 신경안쓰고 진행함.
						LCS[a][b]=LCS[a-1][b-1]+1;
				else{
					if(LCS[a-1][b]>=LCS[a][b-1]) //이거의 =부호는 의미없음 	
						LCS[a][b]=LCS[a-1][b];
					else
						LCS[a][b]=LCS[a][b-1];
				}	
			}
		
		int a=aLine;
		int b=bLine;
		while(LCS[a][b]!=0){
			if(strcasecmp(AfileLine[a],BfileLine[b])==0){ //두문장이 같다면
				checkAsame[a--]=1;
				checkBsame[b--]=1; //체크하고 둘다 한칸씩 뒤로
			}
			else{ //둘이 다른경우
				if(LCS[a-1][b]>LCS[a][b-1])
						a--;
				else
						b--;
			}
		}
	}
	else{ //옵션 i가 꺼진 경우
		for(int a=1;a<=aLine;a++) //최장 공통 부분 문자라인 개수 구하기
			for(int b=1;b<=bLine;b++){
				if(strcmp(AfileLine[a],BfileLine[b])==0)
						LCS[a][b]=LCS[a-1][b-1]+1;
				else{
					if(LCS[a-1][b]>=LCS[a][b-1]) 	
						LCS[a][b]=LCS[a-1][b];
					else
						LCS[a][b]=LCS[a][b-1];
				}
			}
			

		int a=aLine;
		int b=bLine;
		while(LCS[a][b]!=0){
			if(strcmp(AfileLine[a],BfileLine[b])==0){ //두문장이 같다면
				checkAsame[a--]=1;
				checkBsame[b--]=1; //체크하고 둘다 한칸씩 뒤로
			}
			else{ //둘이 다른경우
				if(LCS[a-1][b] > LCS[a][b-1])
						a--;
				else
						b--;
			}
		}
	}


	if((aLine==bLine) && (aLine==LCS[aLine][bLine])) //두 파일 내용이 완전 같은 경우
			isDifferent=0;
	else
			isDifferent=1;

	//필요없어진 배열은 free
	for(int i=0;i<1025;i++)
			free(LCS[i]);

	free(LCS);
	
	//공통부분수열 구했으면, 문단 구하기
	int a=1;
	int b=1;
	int findParagraph=0;
	for( ;a<=(aLine+1) && b<=(bLine+1) ; ){ //조건을 하나 더 올려주는 이유는 혹시 마지막 줄이 최대공통부분수열에 들어가는 경우때문.
		if(checkAsame[a]&&checkBsame[b]){
				if(findParagraph){ //찾았다고 표시하고 문단이 어디까지 이어지는지 체크중이라면 그냥 숫자만 올려감.
						a++;b++;
				}
				else{ //문단 첫 발견인 경우
					findParagraph=1; //문단발견했다고 체크
					Aparagraph[++paragraphNum].startLine=a;
					Bparagraph[paragraphNum].startLine=b;
					a++; b++;
				}
		}
		else if( !checkAsame[a] && checkBsame[b] ){ //a는 공통 부분을 가리키고 있지만 b는 아닌 경우
				if(findParagraph){ //문단발견해서 어디까지 이어지나 찾는중이였던 경우
					findParagraph=0; //문단끝났다고 체크
					Aparagraph[paragraphNum].endLine=a-1;  //이전까지만 같았던 거임.
					Bparagraph[paragraphNum].endLine=b-1;  
				}
				a++;
		}
		else if( checkAsame[a] && !checkBsame[b] ){
				if(findParagraph){ //문단발견해서 어디까지 이어지나 찾는중이였던 경우
					findParagraph=0; //문단끝났다고 체크
					Aparagraph[paragraphNum].endLine=a-1;  //이전까지만 같았던 거임.
					Bparagraph[paragraphNum].endLine=b-1;  
				}
				b++;
		}
		else{
				if(findParagraph){ //문단발견해서 어디까지 이어지나 찾는중이였던 경우
					findParagraph=0; //문단끝났다고 체크
					Aparagraph[paragraphNum].endLine=a-1;  //이전까지만 같았던 거임.
					Bparagraph[paragraphNum].endLine=b-1;  
				}
				a++; b++;
		}
	}
	
	Aparagraph[paragraphNum+1].startLine=aLine+1;
	Aparagraph[paragraphNum+1].endLine=aLine+1;
	Bparagraph[paragraphNum+1].startLine=bLine+1;
	Bparagraph[paragraphNum+1].endLine=bLine+1;
	//가상의 마지막문단 추가	
	
	return isDifferent;
}

void deleteDiff(int previousAend,int nextAstart,int previousBend,int nextBstart){
	if( previousAend+2 == nextAstart ){ //기준파일에서 사라진 문장이 하나인 경우
		printf("%dd%d \n",previousAend+1,previousBend);
		printf("< %s",AfileLine[previousAend+1]); //사라진 내용
	}
	else{ //사라진 문장이 하나가 아닌 경우
		
		printf("%d,%dd%d \n",previousAend+1,nextAstart-1,previousBend);
	
		for(int i=previousAend+1; i<nextAstart; i++){
			printf("< %s",AfileLine[i]);
		}
	}

}

void changeDiff(int previousAend,int nextAstart,int previousBend,int nextBstart){
	if( previousAend+2 == nextAstart ){ //기준파일의 틈이 한문장인 경우
		if(previousBend+2==nextBstart){ //비교파일의 틈도 한문장인 경우
			printf("%dc%d \n",previousAend+1,previousBend+1);
			printf("< %s",AfileLine[previousAend+1]); //바뀔 내용
			printf("---\n");
			printf("> %s",BfileLine[previousBend+1]); //바뀐 내용
		}
		else{ //비교파일은 틈이 한문장이 아닌 경우
			printf("%dc%d,%d \n",previousAend+1,previousBend+1,nextBstart-1);
			printf("< %s",AfileLine[previousAend+1]);
			printf("---\n");
			for(int i=previousBend+1; i<nextBstart; i++){
				printf("> %s",BfileLine[i]); 
			}
		}
	}
	else{ //기준파일의 바뀐 문장이 하나가 아닌 경우
		if(previousBend+2==nextBstart){ //비교파일의 틈은 한문장인 경우
			printf("%d,%dc%d \n",previousAend+1,nextAstart-1,previousBend+1);
			for(int i=previousAend+1;i<nextAstart;i++){
				printf("< %s",AfileLine[i]); //바뀔 내용
			}
			printf("---\n");
			printf("> %s",BfileLine[previousBend+1]); //바뀐 내용
		}
		else{ //비교파일은 틈이 한문장이 아닌 경우
			printf("%d,%dc%d,%d \n",previousAend+1,nextAstart-1,previousBend+1,nextBstart-1);
			for(int i=previousAend+1;i<nextAstart;i++){
				printf("< %s",AfileLine[i]); //바뀔 내용
			}
			printf("---\n");
			for(int i=previousBend+1; i<nextBstart; i++){
				printf("> %s",BfileLine[i]); 
			}
		}
	}
}

void appendDiff(int previousAend,int nextAstart,int previousBend,int nextBstart){
	if( previousAend+1 == nextAstart ){ //기준파일의 틈이 없는걸 체크
		if(previousBend+2==nextBstart){ //비교파일의 틈이 한문장인 경우
			printf("%da%d \n",previousAend,previousBend+1);
			printf("> %s",BfileLine[previousBend+1]); //추가해야할 내용
			//만약 nextBstart가 bLine+1과 같다면 마지막 문장 출력
		}
		else{ //비교파일 틈이 한문장이 아닌 경우
			printf("%da%d,%d \n",previousAend,previousBend+1,nextBstart-1);
			for(int i=previousBend+1; i<nextBstart; i++){
				printf("> %s",BfileLine[i]); 
			}
		}
	}
}

 
void printDiffFile(char* compareA,char* compareB){
	int precentALine=1;
	int precentBline=1;
	
	for(int i=0;i<paragraphNum+1;i++){ //기준파일을 기준으로 두 문단씩 비교할거임.
		if(Aparagraph[i].endLine+1<Aparagraph[i+1].startLine){ //기준파일의 문단 간 틈이 있는 경우
			if(Bparagraph[i].endLine+1<Bparagraph[i+1].startLine){
					//기준파일에 틈이 있는데 비교파일도 문단간 틈이 있는 경우=>change. 사이 틈이 바뀜
					changeDiff(Aparagraph[i].endLine, Aparagraph[i+1].startLine,
									Bparagraph[i].endLine, Bparagraph[i+1].startLine);
			}
			else{   //기준파일은 틈이 있는데 비교파일에는 틈이 없는 경우=>delete. 틈이 사라진것임.
					deleteDiff(Aparagraph[i].endLine, Aparagraph[i+1].startLine,
									Bparagraph[i].endLine, Bparagraph[i+1].startLine);
			}
		}
		else { //기준파일의 두 문단 사이에 틈이 없는경우
			if(Bparagraph[i].endLine+1<Bparagraph[i+1].startLine){
					//기준파일에 틈이 없는데 비교파일은 문단간 틈이 있는 경우=>a. 사이 틈 추가됨.
					appendDiff(Aparagraph[i].endLine, Aparagraph[i+1].startLine,
									Bparagraph[i].endLine, Bparagraph[i+1].startLine);	
			}
			else{   //기준파일에 틈이 없는데 비교파일에도 틈이 없는 경우=> 아무일도 안일어남.
					
			}
		}
	}
}

//두 파일내용이 다르면 1리턴해줄것임
int fileCompare(char* compareA,char* compareB){
	int isDifferent=0; 
	char* readMode="r";
    char bufA[BUFFER_SIZE]; //파일a의 한줄씩 읽기위한 버퍼
	char bufB[BUFFER_SIZE]; //파일b의 한줄씩 읽기위한 버퍼
	FILE* A;
	FILE* B;
	if( ((A=fopen(compareA,readMode))==NULL) || ((B=fopen(compareB,readMode))==NULL) ){
		printf("없는 파일.에러입니다.\n");
		exit(1);
	}
	
	aLine=0; //초기화
	bLine=0; //초기화
	strcpy(AfileLine[0],"");
	strcpy(BfileLine[0],"");
	

	while(fgets(bufA,BUFFER_SIZE,A)!=NULL){
		//엔터도 그래도 가져옴.	
		strcpy(AfileLine[++aLine],bufA);		
	}

	char* endOfFile="\\No newline at end of file\n";
	char lastLine[1025];
	if(AfileLine[aLine][ strlen(AfileLine[aLine]) - 1 ] != '\n'){ //파일의 마지막 줄에 개행이 안붙어 있는 경우	
			strcpy(lastLine,AfileLine[aLine]);
			sprintf(AfileLine[aLine],"%s\n%s",lastLine,endOfFile);
	}
	//어차피 개행여부도 문장이 서로 같냐 안같냐에 영향을 주는 요소이므로
	//본래 같은 문장에 둘 다 개행이 없었다면 해당 문장이 붙을 것이므로 같다는 것에는 변함 없음.
		
	
	//b문장 가져옴
	while(fgets(bufB,BUFFER_SIZE,B)!=NULL){
		//엔터도 그래도 가져옴.	
		strcpy(BfileLine[++bLine],bufB);		
	}
	if(BfileLine[bLine][ strlen(BfileLine[bLine]) - 1 ] != '\n'){ //파일의 마지막 줄에 개행이 안붙어 있는 경우	
			strcpy(lastLine,BfileLine[bLine]);
			sprintf(BfileLine[bLine],"%s\n%s",lastLine,endOfFile);
	}
	
	char commonPartA[4097]; char commonPartB[4097];
	commonPartRemove(compareA,compareB,commonPartA,commonPartB);
	
	if(option['q']){ //파일이 다르면 다르다 알림만주면 됨.
		if(option['i']){ //옵션i 켜져있으면 대소문자 구분안함.
				if(aLine==bLine){
					for(int i=1;i<=aLine;i++)
						if(strcasecmp(AfileLine[i],BfileLine[i])!=0){ //달라진다면 탈출
								isDifferent=1;
								break;			
						}
				}
				else{ //애초에 줄 수가 다름.
					isDifferent=1;
				}

		}
		else{
				if(aLine==bLine){
					for(int i=1;i<=aLine;i++)
						if(strcmp(AfileLine[i],BfileLine[i])!=0){ //달라진다면 탈출
								isDifferent=1;
								break;			
						}
				}
				else{
					isDifferent=1;
				}
			
		}
		
		if( isDifferent==0 && option['s'] ){ 
			//두 파일 내용이 같고, 둘이 같으면 같다고 출력해야 한다면
			printf("Files %s and %s are identical\n",commonPartA,commonPartB);
		}
		else if (isDifferent==0){ //파일 내용은 같으나 옵션s안켜짐.
			//아무일도 안일어나고 파일 비교 종료
		}
		else{ //파일이 다르고 옵션q가 켜진경우. 다르면 다르다고만 출력하기.
			printf("Files %s and %s differ\n",commonPartA,commonPartB);
			isDifferent=1; //다르다고 체크
		}
	}
	else{ //다르면 차이점 출력하고 같으면 옵션s유무에 따라 결정
		
		initLCS();
		if(startLCS()) //LCS를 구했는데 결과가 두 파일이 다르다이면 1을 리턴받게됨.
				isDifferent=1; //두파일 다르다고 설정
		else if(option['s']) //두 파일이 같다이고 만약 옵션 s가 켜진경우 같다고 말만 해주면 됨.
				printf("Files %s and %s are identical\n",commonPartA,commonPartB);

		if(isDifferent && isDirectoryCompare){
				//두 파일이 다르고 만약 해당 파일비교를 요청한곳이 DirectoryCompare함수에서 비롯된 것이라면
				char opt[5];
				int optionNum=-1;
				if(option['q']) opt[++optionNum]='q';
				if(option['s']) opt[++optionNum]='s';
				if(option['i']) opt[++optionNum]='i';
				if(option['r']) opt[++optionNum]='r';
				
				if(optionNum==-1) //옵션 개수가 없는 경우
						printf("diff %s %s\n",commonPartA,commonPartB);
				else{
						opt[++optionNum]='\0';
						printf("diff -%s %s %s\n",opt,commonPartA,commonPartB);
				}
		}
		printDiffFile(compareA,compareB); //두 파일이 같으면 기본적으로 출력되는 내용 없음.
		free(Aparagraph);
		free(Bparagraph);
		fclose(A);
		fclose(B);
	}
	return isDifferent;
}

int ASCIIpriority(char* nameA,char* nameB){ //A가 우선순위가 높으면 1을 리턴, 아니면 0리턴 , 둘이 아예 같으면 2리턴.
		int minStrLength=0;
		if(strlen(nameA)>=strlen(nameB))
				minStrLength=strlen(nameB); //nameB의 이름이 더 짧은 경우
		else				
				minStrLength=strlen(nameA); //nameA의 이름이 더 짧은 경우

		for(int i=0; i<minStrLength;i++){
				if(nameA[i]<nameB[i]) //nameA의 우선순위가 더 높은 경우
						return 1;
				else if(nameA[i]>nameB[i])
						return 0;
		}
		//여기까지 코드흐름이 왔다는 것은 [minStrLength-1]까지는 서로 같다는 것.
		if(strlen(nameA)==strlen(nameB)) //둘이 사실 아예 같은 문자열이였던 경우
				return 2;
		else if(nameA[minStrLength]=='\0') //A는 끝이났는데 둘의 문자열이 다르다는 것은 B까 더 길다는 것.
				return 1; //그러면 A가 더 짧으므로 A우선순위가 더 높음.
		else
				return 0; 

}



//두 디렉토리가 다르면 1리턴. 재귀적으로 하든 뭐든 하위에서 다른게 있으면 1리턴함.
int directoryCompare(char* compareA,char* compareB){
		int isDifferent=0;
		struct dirent** fileListA=NULL;
		struct dirent** fileListB=NULL;
		int numInDirA,numInDirB;
		numInDirA=0; //디렉토리 안에 파일 혹은 서브 디렉토리 개수를 리턴받을 변수
		numInDirB=0;
		numInDirA=scandir(compareA,&fileListA,NULL,alphasort); //파일리스트 불러옴
		numInDirB=scandir(compareB,&fileListB,NULL,alphasort);
		char compareSameNameFileA[4096];
		char compareSameNameFileB[4096];
		int* checkNameA=malloc(sizeof(int)*numInDirA);
		int* checkNameB=malloc(sizeof(int)*numInDirB);
		memset(checkNameA,0,sizeof(int)*numInDirA);//초기화
		memset(checkNameB,0,sizeof(int)*numInDirB);//초기화
		int i=0,j=0; //아스키 우선순위로 비교하기위해
		for(;(i<numInDirA) && (j<numInDirB);){
				int ascPriority=ASCIIpriority(fileListA[i]->d_name, fileListB[j]->d_name); 
				if( ascPriority==2 ){
					//두 파일명이 같음.
					if(strcmp(fileListA[i]->d_name,".") && strcmp(fileListA[i]->d_name,"..")){ 
						//파일명이 같은데 파일명이 . ..이 아닌경우
						
						//디렉토리명이랑 파일이름 /로 이어주기
						nextPathPlus(compareSameNameFileA,compareA,fileListA[i]->d_name);
						nextPathPlus(compareSameNameFileB,compareB,fileListB[j]->d_name);
						
						//두파일(디렉토리)에 대한 절대경로 중 공통부분 제외하고 뽑아내기
						char commonPartA[4097]; char commonPartB[4097];
						commonPartRemove(compareSameNameFileA,compareSameNameFileB,commonPartA,commonPartB);

						struct stat bufA;
						struct stat bufB;
						if( (lstat(compareSameNameFileA,&bufA)<0) || (lstat(compareSameNameFileB,&bufB)<0) ){
							fprintf(stderr,"파일 정보 못불러옴\n");
							exit(1);
						}
					
						else{
							int Astate=-1;
							int Bstate=-1;
							if(S_ISREG(bufA.st_mode))
									Astate=1;
							else if(S_ISDIR(bufA.st_mode))
									Astate=2;
	
							if(S_ISREG(bufB.st_mode))
									Bstate=1;
							else if(S_ISDIR(bufB.st_mode))
									Bstate=2;
	
	
							if(Astate==Bstate && Astate==1){ //두파일이 이름이 같은 정규파일인 경우
									if(fileCompare(compareSameNameFileA,compareSameNameFileB)){
											isDifferent=1; //두 파일의 내용이 다르면 1로 설정.
									}
							}
							else if(Astate==Bstate && Astate==2){ //파일명같고 디렉토리인 경우
									if(option['r']){
										
											if(directoryCompare(compareSameNameFileA,compareSameNameFileB))
													isDifferent=1; //해당 디렉토리가 다르면 다르다고 체크 
											else{ 
												//재귀적으로 돌아서 두 디렉토리가 완전 같으면 같다고 알림. 
												printf("Common subdirectories: %s and %s\n",
																	commonPartA,
																	commonPartB);	
											}
											
									}
									else	
											printf("Common subdirectories: %s and %s\n",commonPartA,commonPartB);	
									
							}
							else{ //파일명은 같으나 타입이 다른경우
								if(Astate==1){ //A가 정규파일이므로 B는 디렉토리
										printf("File %s is a regular file while file %s is a directory \n",
														commonPartA,commonPartB);
								}
								else if(Astate==2){ //A가 디렉토리이고 B가 정규파일인 경우									
										printf("File %s is a directory while file %s is a regular file \n",
														commonPartA,commonPartB);		
								}

								isDifferent=1;  //두 디렉토리의 차이점을 발견했으므로 1로 설정
							}
						
						}
					}
			    	//일단 이름이 같으면 처리하고나서 둘 다 넘어감.
					free(fileListA[i]);
					free(fileListB[j]);
					i++; j++; 
		    	}
			    else if(ascPriority==1){ //이름이 다르고 A가 우선순위인 경우
				
					char commonPartA[4097]; char commonPartB[4097];
					commonPartRemove(compareA,compareB,commonPartA,commonPartB);
						
			         //이름이 같은게 없어서 여기까지 오게 된거임.
			        printf("Only in %s: %s\n",commonPartA,fileListA[i]->d_name);
			    	free(fileListA[i]);
				    i++;
			     	isDifferent=1; //두 디렉토리의 구조가 다름
		    	}
			    else{ //이름이 같은게 없고 B가 우선순위인 경우
					
					char commonPartA[4097]; char commonPartB[4097];
					commonPartRemove(compareA,compareB,commonPartA,commonPartB);

				    printf("Only in %s: %s\n",commonPartB,fileListB[j]->d_name);
			      	free(fileListB[j]);
			     	j++;
			    	isDifferent=1; //두 디렉토리의 구조가 다름
		     	}
		}
		
		if( (i!=numInDirA) || (j!=numInDirB) )
				isDifferent=1; //둘이 다르다는 것은 디렉토리간에 차이점이 있다는 것임.


		
		char commonPartA[4097]; char commonPartB[4097];
		commonPartRemove(compareA,compareB,commonPartA,commonPartB);

		//이제 남은거 처리해주면 됨.
		if(i==numInDirA) //A는 다소비했음. B소비시켜주면 됨.
			for(;j<numInDirB;j++){
				printf("Only in %s: %s\n",commonPartB,fileListB[j]->d_name);
				free(fileListB[j]);
			}
		
		if(j==numInDirB) //B는 다소비했음. A소비시켜주면 됨.
			for(;i<numInDirA;i++){
				printf("Only in %s: %s\n",commonPartA,fileListA[i]->d_name);
				free(fileListA[i]);
			}
		
		free(fileListA);
		free(fileListB);

		return isDifferent;
}

void commandExit(){	
	gettimeofday(&end_t,NULL);
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

	printf("Prompt End\n");
	printf("Runtime: %ld:%06ld(sec:usec)\n",sec,usec);
	exit(0);
}

void commandHelp(){
	printf("Usage:\n");
	printf("  > find [FILENAME] [PATH]\n");
	printf("     >> [INDEX] [OPTION ... ]\n");
	printf("  > help\n");
	printf("  > exit\n\n");
	printf("  [OPTION ... ]\n");
	printf("   q : report only when files differ\n");
	printf("   s : report when two files are the same\n");
	printf("   i : ignore case differences in file contents\n");
	printf("   r : recursively compare any subdirectories found\n");
}

void changeStatTime(char* _time,struct tm* t){
	char str[15];
	sprintf(str,"%02d-%02d-%02d %02d:%02d",t->tm_year + 1900 - 2000, 
										t->tm_mon + 1, 
										t->tm_mday,
									    t->tm_hour, 
										t->tm_min);
	strcpy(_time,str);
}

void changeStatMode(char* c,mode_t m){
	char mode[11]="drwxrwxrwx"; //문자열보다 하나 더 크게 11배열로 잡기.
	if((m&S_IFMT)==S_IFREG) //정규파일이면 d를 -으로 표시
		mode[0]='-';
	for(int i=0;i<9;i++){
		if((m&1)==0) 
			mode[9-i]='-';
		
		m=(m>>1); //1자리만큼 이동시킴. 비트마스크연산
	}
	
	strcpy(c,mode);
}


void listPrint(){

	printf("Index\tSize\tMode\t\tBlocks\tLinks\tUID\tGID\tAccess\t\tChange\t\tModify\t\tPath\t\n");
    
	char m[11];
	char time[15];

	for(int i=0;i<listIndex;i++){
	   	printf("%d\t",i); //인덱스
	   	printf("%ld\t",sameNameSizeList[i].statInfo.st_size); //사이즈
		changeStatMode(m,sameNameSizeList[i].statInfo.st_mode);//모드변환 함수
	   	printf("%s\t",m); //모드
	   	printf("%ld\t",sameNameSizeList[i].statInfo.st_blocks); //Blocks==할당된 블럭수
	   	printf("%lu\t",sameNameSizeList[i].statInfo.st_nlink); //링크스
	   	printf("%d\t",sameNameSizeList[i].statInfo.st_uid); //UID
	   	printf("%d\t",sameNameSizeList[i].statInfo.st_gid); //GID
	   	changeStatTime(time, localtime(&sameNameSizeList[i].statInfo.st_atime));
		printf("%s\t",time); 
		//Access==최종 접근 시간
		changeStatTime(time,localtime(&sameNameSizeList[i].statInfo.st_ctime));
	   	printf("%s\t",time); 
		//Change==최종 상태변경시간
		changeStatTime(time,localtime(&sameNameSizeList[i].statInfo.st_mtime));
	   	printf("%s\t",time); 
		//Modify==최종 수정 시간
		printf("%s\t\n",sameNameSizeList[i].path); //경로	
	}	
}
