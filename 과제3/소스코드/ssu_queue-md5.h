char TrashPath[PATHMAX]; //쓰레기통 경로.
char TrashInfoPath[PATHMAX]; //쓰레기통 정보 디렉토리.
char logFile[PATHMAX];
int fd_log;
struct passwd* passwd_p;

typedef struct Node{
		char* path; //
		struct Node* next; //자기자신을 아직 컴파일러는 Node라는 이름으로 모름..
}Node;

typedef struct Queue{
		Node* front; 
		Node* rear;
		int count; //큐의 노드 개수
		off_t size; //해당 파일의 크기.
		char md[HASHMAX]; //해당 큐가 담당하는 해쉬값.
		struct Queue* next; //Root구조체에서 쓸것임. 큐는 자신의 next멤버를 직접  안씀.
}Queue;


typedef struct Root{
		Queue* front;
		Queue* rear;
		int count; 
}Root;

void printroot(Root* r);

void get_time(time_t stime,char* recieveString){
		char* time = (char*)malloc(sizeof(char) * BUFMAX);
		struct tm *tm;
		tm = localtime(&stime);
		sprintf(time, "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
		strcpy(recieveString,time);
		free(time);
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

void get_size(off_t fSize,char* recieveString){
		char* size=(char*)malloc(sizeof(char)*100);
		char* sizeString=(char*)malloc(sizeof(char)*100);
		sprintf(sizeString,"%ld",fSize); //일단 문자열로 변환.

		int length=strlen(sizeString);
		int m=length%3;

		int index=0;
		for(int i=0;i<length;i++){
				if( (i!=0) && (i%3==m) ){
						size[index++]=',';
				}

				size[index++]=sizeString[i];
		}	
		size[index]='\0';
		strcpy(recieveString,size);
		free(size);
		free(sizeString);
}

void initqueue(Queue* q,off_t fSize,char* md5){
		q->front=NULL;
		q->rear=NULL;
		q->count=0; //큐에 있는 노드의 수.
		q->size=fSize; //해당 큐에 들어갈 파일들의 크기
		memset(q->md,0,HASHMAX);
		strcpy(q->md,md5);
		q->next=NULL;
}

void enqueue(Queue* q,char* path){
		Node* newNode=(Node*)malloc(sizeof(Node));
		newNode->path=(char*)malloc(sizeof(char)*strlen(path)+1);
		strcpy(newNode->path,path); 
		newNode->path[strlen(path)]='\0';
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

//큐를 FIFO 구조로..사용할 때 맨 앞 노드를리턴하는 함수.
void dequeue(Queue* q,char* returnPath){ 
		if(q->count==0) 
				returnPath=NULL; //큐가 비었으면 돌려줄게 없음
		else{
				strcpy(returnPath,q->front->path);
				Node* n=q->front; //임시로 현재의 front를 받아줄 놈
				q->front=q->front->next; //front는 다음 노드로 넘어감
				free(n->path); //노드안의 경로 해제
				free(n); //프리해서 해제
				q->count=(q->count)-1;
		}
}

void deletequeue(Queue* q){
		if(q==NULL)
				return;
		Node* cur=q->front;
		while(cur!=NULL){
				Node* n=cur; //임시로 현재의 front를 받아줄 놈
				q->front=cur->next; //front는 다음 노드로 넘어감
				cur=cur->next;
				free(n->path); //노드 안의 경로 해제
				free(n); //프리해서 해제
				q->count=(q->count)-1;
		}
		free(q);
}

int countQueue(Queue* q){
		return q->count;
}

int isEmpty(Queue* q){
		return q->count==0?1:0;
}


void initroot(Root* root){
		root->front=NULL;
		root->rear=NULL;
		root->count=0;
}

void enroot(Root* r,char* path,off_t fSize,char* md5){

		if(r->count==0){
				Queue* newQ=(Queue*)malloc(sizeof(Queue));
				initqueue(newQ,fSize,md5); //큐초기화
				enqueue(newQ,path); //큐에 삽입.
				r->front=newQ;
				r->rear=newQ;
				r->count ++;
		}
		else
		{		
				//printf("여기 진입\n");
				Queue* indexQ=r->front;
				int findQueue=0;
				while(indexQ!=NULL){
						if(!strcmp(indexQ->md,md5)){
								findQueue=1; //큐를 찾았으면 1로
								enqueue(indexQ,path);
								break;
						}

						indexQ=indexQ->next;
				}
				//큐를 못찾음... 새로 루트 꼬리에 새로운 큐 넣어주기.
				if(findQueue==0){
						Queue* newQ=(Queue*)malloc(sizeof(Queue));
						initqueue(newQ,fSize,md5); //큐초기화
						enqueue(newQ,path); //해당큐에 삽입.
						r->rear->next=newQ;
						r->rear=newQ;
						r->count ++;
				}
		}

}

void insert_sub_root(Root* r, Root* subr){
		if(subr->count!=0){
				if(r->rear==NULL){
						r->front=subr->front;
						r->rear=subr->rear;
				}
				else{
						r->rear->next=subr->front; //이어주기.
						r->rear=subr->rear;
				}

				r->count= r->count + subr->count;
		}
}


void restore_enroot(Root* r,Root* collect,char* path,char* md5){

		Queue* indexQ=r->front;
		int findQueue=0;
		for(int i=0; i < r->count ; i++){
				int same=1;

				if(!strcmp(indexQ->md,md5)){
						findQueue=1; //큐를 찾았으면 1로
						enqueue(indexQ,path);
						break;
				}

				indexQ=indexQ->next;
		}

		//기존의 링크드리스트에서는 못찾음... 새로 루트 꼬리에 새로운 큐 넣어주기.
		if(findQueue==0){
						
			Queue* indexQ_c=collect->front;
			Queue* previousQ=NULL;
			int findQueue_c=0;
			for(int i=0; i < collect->count ; i++){
					int same_c=1;
					
					if(!strcmp(indexQ_c->md,md5)){
							findQueue_c=1; //큐를 찾았으면 1로
							enqueue(indexQ_c,path);
							break;
					}
					previousQ=indexQ_c;
					indexQ_c=indexQ_c->next;
			}

			if(findQueue_c){
				if(r->count!=0){
					r->rear->next=indexQ_c;
					r->rear=indexQ_c;
					if(previousQ==NULL){
						collect->front=indexQ_c->next;
						if(indexQ_c->next==NULL){ // 맨 뒷 놈이였다면
							collect->rear=previousQ;
						}
					}
					else{
						previousQ->next=indexQ_c->next;
						if(indexQ_c->next==NULL){ // 맨 뒷 놈이였다면
							collect->rear=previousQ;
						}
					}
					indexQ_c->next=NULL; //기존 컬렉터와 연결을 끊고 루트로 들어감
					collect->count --;
				}
				else{
					r->front=indexQ_c;
					r->rear=indexQ_c;
					if(previousQ==NULL){
						collect->front=indexQ_c->next;
						if(indexQ_c->next==NULL){ // 맨 뒷 놈이였다면
							collect->rear=previousQ;
						}
					}
					else{
						previousQ->next=indexQ_c->next;
						if(indexQ_c->next==NULL){ // 맨 뒷 놈이였다면
							collect->rear=previousQ;
						}
					}
					indexQ_c->next=NULL; //기존 컬렉터와 연결을 끊고 루트로 들어감
					collect->count --;
				}

				r->count++;
			}
		}

}


void collect_one_set(Root* r, Root* collect){
	Queue* q=r->front;
	while(q!=NULL){
		if(q->count==1){
			enroot(collect, q->front->path, q->size, q -> md);
		}
		q=q->next;
	}
}

//수정할 세트인덱스와 옵션상태,삭제할인덱스들의 정보
void deroot(Root* r,Root* collect,int setNum,char option,int* deleteIndex){ //root rear 수정잘해줘라.

		Queue* q=r->front; //여기가 실제로 세트 모음들 중 1번째 세트임.
		for(int i=1;i<setNum;i++){
				q=q->next;
		}
		//원하는 세트의 큐를 q가 갖고있음.


		int qCount=q->count;

		if(option=='d'){
				Node* previousN=NULL;
				Node* n=q->front;
				Node* freeN=NULL;
				for(int i=0; i<qCount; i++){
						if(deleteIndex[i]==1){ //지워야하는 놈이면...
								if(previousN==NULL){ //
										freeN=n; //기억.
										q->front=n->next;
										printf("\"%s\" has been deleted in #%d\n",n->path,setNum);
										unlink(n->path); //삭제.

										//로그 쓰기
										time_t delete_time=time(NULL);
										struct tm* delete_t=localtime(&delete_time);		
										char log[10000];
										sprintf(log,"[DELETE] %s %d-%d-%d %d:%d:%d %s\n",freeN->path,
														delete_t->tm_year+1900, delete_t->tm_mon+1, delete_t->tm_mday,
														delete_t->tm_hour, delete_t->tm_min, delete_t->tm_sec,
														passwd_p->pw_name);
										write(fd_log,log,strlen(log));


										free(freeN->path);
										free(freeN);
										n=q->front; //n한칸 전진.
								}
								else{
										freeN=n;
										previousN->next=n->next; //n제거하기 전에 이어주기.
										printf("\"%s\" has been deleted in #%d\n",n->path,setNum);
										unlink(n->path); //삭제.

										//로그 쓰기
										time_t delete_time=time(NULL);
										struct tm* delete_t=localtime(&delete_time);		
										char log[10000];
										sprintf(log,"[DELETE] %s %d-%d-%d %d:%d:%d %s\n",freeN->path,
														delete_t->tm_year+1900, delete_t->tm_mon+1, delete_t->tm_mday,
														delete_t->tm_hour, delete_t->tm_min, delete_t->tm_sec,
														passwd_p->pw_name);
										write(fd_log,log,strlen(log));


										free(freeN->path);
										free(freeN);
										//이때는 previous 내비두기.
										n=previousN->next; //n한칸 전진.
								}
								q->count --;
						}
						else{
								previousN=n;
								n=n->next;
						}
				}

				if(previousN!=NULL){
		                  previousN->next=NULL;
		                  q->rear=previousN;
		        }
  
		        if(q->count==0){
				   		  q->front=NULL;
						  q->rear=NULL;
				}

		}
		else if(option=='i'){
				Node* previousN=NULL;
				Node* n=q->front;
				Node* freeN=NULL;
				for(int i=0; i<qCount; i++){
						if(deleteIndex[i]==1){ //지워야하는 놈이면...
								if(previousN==NULL){ //문제되는지 확인하기.
										freeN=n; //기억.
										q->front=n->next;
										unlink(n->path);

										//로그 쓰기
										time_t delete_time=time(NULL);
										struct tm* delete_t=localtime(&delete_time);		
										char log[10000];
										sprintf(log,"[DELETE] %s %d-%d-%d %d:%d:%d %s\n",freeN->path,
														delete_t->tm_year+1900, delete_t->tm_mon+1, delete_t->tm_mday,
														delete_t->tm_hour, delete_t->tm_min, delete_t->tm_sec,
														passwd_p->pw_name);
										write(fd_log,log,strlen(log));

										free(freeN->path);
										free(freeN);
										n=q->front; //n한칸 전진.
								}
								else{
										freeN=n;
										previousN->next=n->next; //n제거하기 전에 이어주기.
										unlink(n->path);

										//로그 쓰기
										time_t delete_time=time(NULL);
										struct tm* delete_t=localtime(&delete_time);		
										char log[10000];
										sprintf(log,"[DELETE] %s %d-%d-%d %d:%d:%d %s\n",freeN->path,
														delete_t->tm_year+1900, delete_t->tm_mon+1, delete_t->tm_mday,
														delete_t->tm_hour, delete_t->tm_min, delete_t->tm_sec,
														passwd_p->pw_name);
										write(fd_log,log,strlen(log));


										free(freeN->path);
										free(freeN);
										//이때는 previous 내비두기.
										n=previousN->next; //n한칸 전진.
								}
								q->count --;
						}
						else{
								previousN=n;
								n=n->next;
						}
				}
				
				if(previousN!=NULL){
						 previousN->next=NULL;
						 q->rear=previousN;
				}
				  
				if(q->count==0){
						q->front=NULL;
						q->rear=NULL;
				}

		}
		else if(option=='f'){
				struct stat buf;
				Node* n=q->front;
				time_t max; //가장 최근에 수정한 파일의 값을 기억할 것임.
				if(lstat( n->path , &buf )<0){
						printf("error\n");
						return;
				}

				Node* remainNode=n; //남길 노드.
				max=buf.st_mtime;
				while(n!=NULL){
						if(lstat(n->path,&buf)<0){
								printf("error\n");
								return;
						}
						if(max < buf.st_mtime){
								remainNode=n; //기억.
								max=buf.st_mtime;
						}
						n=n->next;
				}

				//남길 노드를 맨앞으로 위치 변경.
				if(remainNode!=q->front){
						n=q->front;
						char* TMPpath=n->path; //임시 기억용.
						n->path=remainNode->path;
						remainNode->path=TMPpath;
				}


				n=q->front->next;
				q->front->next=NULL;

				q->rear=q->front;
				Node* freeN=n;
				while(n!=NULL){
						freeN=n;
						n=n->next;
						unlink(freeN->path); //삭제

						//로그 쓰기
						time_t delete_time=time(NULL);
						struct tm* delete_t=localtime(&delete_time);		
						char log[10000];
						sprintf(log,"[DELETE] %s %d-%d-%d %d:%d:%d %s\n",freeN->path,
										delete_t->tm_year+1900, delete_t->tm_mon+1, delete_t->tm_mday,
										delete_t->tm_hour, delete_t->tm_min, delete_t->tm_sec,
										passwd_p->pw_name);
						write(fd_log,log,strlen(log));


						free(freeN->path);
						free(freeN); //해제
				}
				q->count=1;
				char changeMtime[100];
				get_time(max,changeMtime);
				printf("Left file in #%d : \"%s\" (%s)\n",setNum,q->front->path,changeMtime);

		}
		else if(option=='t'){ //쓰레기통으로 보내야함.....
				struct stat buf;
				Node* n=q->front;
				time_t max; //가장 최근에 수정한 파일의 값을 기억할 것임.
				if(lstat( n->path , &buf )<0){
						printf("error\n");
						return;
				}

				Node* remainNode=n; //남길 노드.
				max=buf.st_mtime;
				while(n!=NULL){
						if(lstat(n->path,&buf)<0){
								printf("error\n");
								return;
						}
						if(max < buf.st_mtime){
								remainNode=n; //기억.
								max=buf.st_mtime;
						}
						n=n->next;
				}

				//남길 노드를 맨앞으로 위치 변경.
				if(remainNode!=q->front){
						n=q->front;
						char* TMPpath;
						TMPpath=n->path;
						n->path=remainNode->path; 
						remainNode->path=TMPpath;
				}


				n=q->front->next;
				q->front->next=NULL;
				q->rear=q->front;
				Node* freeN=n;

				while(n!=NULL){
						struct dirent** fileList=NULL;
						int numInDir;
						if((numInDir=scandir(TrashPath,&fileList,NULL,alphasort))==-1){ // 쓰레기통의 파일리스트 불러옴.
								fprintf(stderr,"error: 디렉토리를 열 수 없습니다.\n");
								return;
						}

						freeN=n; //삭제할건 얘가 기억.
						n=n->next; //일단 넘어감. 
						char filename[PATHMAX]; //
						char newPath[PATHMAX]; //얘는 수정 안해도 될듯함.
						char newFileName[PATHMAX]; //

						char originPath[PATHMAX];
						strcpy(originPath,freeN->path); //원본 경로 저장.

						findNameFunc(freeN->path,filename); // filename에는 진짜 순수한 파일의 이름만 들어있다.
						strcpy(newFileName,filename);

						char* tmp=strrchr(filename,'.'); //순수 파일이름에서 .만 가져옴.
						char ext[4097]; // 마지막 .을 포함한 이후의 내용.
						memset(ext,'\0',4097);
						char exceptExt[4097]; //마지막 .의 앞 내용
						if(tmp!=NULL){
								strcpy(ext,tmp); //확장자를 가져옴.
								for(int i=0;i<strlen(filename);i++){
										if(tmp==&filename[i]){ // 즉, exceptExt에는 . 이전까지의 파일이름만 남는다.
												exceptExt[i]='\0';
												break;
										}
										else{
												exceptExt[i]=filename[i];
										}
								}
						}


						int nameNum=0;
						int isNameSame=0;
						while(1){
								isNameSame=0;
								for(int i=0;i<numInDir;i++){ //기존 쓰레기통에 같은 이름인 파일이있다면...
										if(!strcmp(fileList[i]->d_name,newFileName)){
												if(strlen(ext)!=0){
														sprintf(newFileName,"%s.%d%s",exceptExt,++nameNum,ext); // 확장자제거파일명.숫자.확장자 
												}
												else{
														//파일에 확장자가 없으면 그냥 이렇게 해도 됨....
														sprintf(newFileName,"%s.%d",filename,++nameNum);
												}
												isNameSame=1;
												break;
										}
								}
								if(isNameSame==0)
										break;
						}

						char InfoPath[PATHMAX];
						if(nameNum==0){
								sprintf(newPath,"%s/%s",TrashPath,filename); //그냥 그대로 저장한면 된다...
								sprintf(InfoPath,"%s/%s%s",TrashInfoPath,filename,"info"); //쓰레기통정보디렉토리에 해당이름+"info"의 파일생성.
						}
						else{ //버전이 새로 생겼다면 새파일이름으로 쓰레기통에 저장.
								sprintf(newPath,"%s/%s",TrashPath,newFileName);
								sprintf(InfoPath,"%s/%s%s",TrashInfoPath,newFileName,"info"); //쓰레기통정보디렉토리에 해당이름+"info"의 파일생성.
						}

						//쓰레기통에 추가한 파일에 데이터 쓰는중.
						int fd;
						if( (fd=open(InfoPath,O_CREAT|O_RDWR|O_TRUNC,0644)) < 0 ){
								fprintf(stderr,"%s open_error\n",InfoPath);
								exit(1);
						}	

						time_t delete_time=time(NULL);
						struct tm* delete_t=localtime(&delete_time);
						//쓰레기통 정보에 저장.
						char writeContents[10000];
						sprintf(writeContents,"%s %s %ld\n",originPath,newPath,delete_time);
						write(fd,writeContents,strlen(writeContents));

						close(fd); //파일디스크립터 닫아주기....

						if(link(freeN->path,newPath)<0)
								printf("can't move in Trash....\n");
						unlink(freeN->path); //삭제....원래 쓰레기통으로 보내야함.


						char log[10000];
						sprintf(log,"[REMOVE] %s %d-%d-%d %d:%d:%d %s\n",originPath,
										delete_t->tm_year+1900, delete_t->tm_mon+1, delete_t->tm_mday,
										delete_t->tm_hour, delete_t->tm_min, delete_t->tm_sec,
										passwd_p->pw_name);

						write(fd_log,log,strlen(log));
						free(freeN->path);
						free(freeN); //해제
						for(int i=0;i<numInDir;i++){
								free(fileList[i]);
						}
						free(fileList);
				}
				q->count=1;

				char changeMtime[100];
				get_time(max,changeMtime);
				printf("All files in #%d have moved to Trash except \"%s\" (%s)\n",setNum,q->front->path,changeMtime);

		}

		collect_one_set(r,collect); //delete 명령어 이후 한개 남은 세트들을 수집.

}

void cleanroot(Root* r){
		Queue* q=r->front;
		Queue* previousQ=NULL;
		Queue* freeQ=NULL;

		while(q!=NULL){
				if(q->count<2){
						if(previousQ==NULL){
								freeQ=q;
								q=q->next;
								r->front=q;
								deletequeue(freeQ);
						}
						else{
								freeQ=q;
								q=q->next;
								previousQ->next=q;
								deletequeue(freeQ);
						}
						r->count--;
				}
				else{
						previousQ=q;
						q=q->next;
				}
		}

		if(previousQ!=NULL){
				previousQ->next=NULL;
				r->rear=previousQ;
		}

		if(r->count==0){
				r->front=NULL;
				r->rear=NULL;
		}
}

void swapNdata(Node* n1,Node* n2){
		char* tmpPath=n1->path;
		n1->path=n2->path;
		n2->path=tmpPath;
}

void swqpQdata(Queue* q1,Queue* q2){

		//next빼고 다 바꿈.
		Node* TMPfront=q1->front;
		Node* TMPrear=q1->rear;
		int TMPcount=q1->count;
		off_t TMPsize=q1->size;
		char TMPmd[HASHMAX];
		memset(TMPmd,0,HASHMAX);
		strcpy(TMPmd,q1->md);

		q1->front=q2->front;
		q1->rear=q2->rear;
		q1->count=q2->count;
		q1->size=q2->size;
		strcpy(q1->md,q2->md);

		q2->front=TMPfront;
		q2->rear=TMPrear;
		q2->count=TMPcount;
		q2->size=TMPsize;
		strcpy(q2->md,TMPmd);
}

void sortroot(Root* r){

		int count=r->count;
		for(int i=0;i < count-1;i++){
				Queue* noSortQ=r->front;
				for(int j=0;j<count-i-1;j++){
						if(noSortQ->size > noSortQ->next->size){
								//앞에서부터 사이즈가 큰 걸 뒤로 보냄.
								//사이즈가 같다면 둘 바꾸지 않음.
								//Root* r에 있는 링크드리스트 Queue들의 각 첫번째 노드들이
								//들어가는 원리를 생각하면 각 큐들의 첫 노드들만 본다면 경로명 순으로
								//정렬이 되어 있는 것을 알 수 있다. 따라서 경로명으로 정렬된 것을
								//크기순으로 다시 정렬하는데 이때 크기가 같으면 뒤에 있는 놈을 먼저
								//뒤로 보낸다고 생각하면, 이 경로명 순은 같은 크기인 세트들에 한해서
								//바뀌지 않음.
								swqpQdata(noSortQ,noSortQ->next);
						}

						noSortQ=noSortQ->next;
				}
		}
}

//pathA가 pathB보다 우선순위가 낮다면....
int comparePath(char* pathA,char* pathB){
		int depthA=0;
		int depthB=0;
		for(int i=0;i<strlen(pathA);i++)
				if(pathA[i]=='/')
						depthA++;

		for(int i=0;i<strlen(pathB);i++)
				if(pathB[i]=='/')
						depthB++;

		if(depthA < depthB){ // pathA가 루트와 더 가까우므로 pathA가 더 루트에 가까움.
				return 1;
		}
		else if(depthA > depthB){
				return -1;
		}
		else{ //깊이가 같다면 문자 하나하나를 비교해봐야함.

				int whoIsBig=strcmp(pathA,pathB);	
				if(whoIsBig<0)
						return 1;
				else
						return -1;
		}
}

//listType==0이면 파일세트로, 1이면 파일리스트를 정렬시킨다. 
void list_root(Root* r,int listType,char* category, int order){
		int count=r->count;
		// 세트 단위로 정렬시킴.
		if(listType==0){
				if(!strcmp(category,"filename")){ //?? 세트의 파일이름 별 정렬이란?

						//세트를 파일이름 순으로?? 정렬할게 없다...
				}
				else if(!strcmp(category,"size")){

						if(order>0){			

								Queue* noSortQ=r->front;
								for(int i=0;i < count-1;i++){
										Queue* findQ=noSortQ;
										for(int j=0;j<count-i-1;j++){
												if(findQ->size > findQ->next->size){ //내림차순에 어긋난다.
														swqpQdata(findQ,findQ->next);
												}
												findQ=findQ->next;
										}
								}


						}
						else{

								Queue* noSortQ=r->front;
								for(int i=0;i < count-1;i++){
										Queue* findQ=noSortQ;
										for(int j=0;j<count-i-1;j++){
												if(findQ->size < findQ->next->size){ //내림차순에 어긋난다.
														swqpQdata(findQ,findQ->next);
												}
												findQ=findQ->next;
										}
								}

						}

				}
				else if(!strcmp(category,"uid")){
						//할게 없다..
				}
				else if(!strcmp(category,"gid")){
						//할게 없다..
				}
				else{ //mode순서로
						//할게 없다...
				}

		}
		else{ // 파일리스트단위로 정렬시킴.

				if(!strcmp(category,"filename")){ //?? 세트의 파일이름 별 정렬이란?

						if(order>0){ //오름차순	

								Queue* noSortQ=r->front; //시작 세트.
								for(int i=0; i<count; i++){ //모든 세트를 돌겠다. 
										Node* noSortN=noSortQ->front; //이번에 정렬할 세트의 시작 노드를 가리킴.
										for(int j=0;j<(noSortQ->count)-1;j++){ //버블정렬은 1개 적게 하면 된다.
												Node* findN=noSortN; //탐색을 시작하면서 버블정렬을 할 
												for(int k=0; k<(noSortQ->count)-j-1; k++){ //이번엔 j번째 인덱스의 노드부터 버블시작.
														if(comparePath(findN->path,findN->next->path) < 0) //순서가 잘못됐으면..
																swapNdata(findN,findN->next);

														findN=findN->next; //다음버블을 위해 노드 위치 이동.
												}
										}
										noSortQ=noSortQ->next; //다음 세트로 이동.
								}	

						}
						else{ 

								Queue* noSortQ=r->front; //시작 세트.
								for(int i=0; i<count; i++){ //모든 세트를 돌겠다. 
										Node* noSortN=noSortQ->front; //이번에 정렬할 세트의 시작 노드를 가리킴.
										for(int j=0;j<(noSortQ->count)-1;j++){ //버블정렬은 1개 적게 하면 된다.
												Node* findN=noSortN; //탐색을 시작하면서 버블정렬을 할 
												for(int k=0; k<(noSortQ->count)-j-1; k++){ //이번엔 j번째 인덱스의 노드부터 버블시작.
														if(comparePath(findN->path,findN->next->path) > 0) //순서가 잘못됐으면..
																swapNdata(findN,findN->next);

														findN=findN->next; //다음버블을 위해 노드 위치 이동.
												}
										}
										noSortQ=noSortQ->next; //다음 세트로 이동.
								}	

						}

				}
				else if(!strcmp(category,"size")){ 
						//같은 세트에는 같은 크기의 파일들만 들어있음..... 이거는 할게 없다.
				}
				else if(!strcmp(category,"uid")){

						if(order>0){

								Queue* noSortQ=r->front; //시작 세트.
								for(int i=0; i<count; i++){ //모든 세트를 돌겠다. 
										Node* noSortN=noSortQ->front; //이번에 정렬할 세트의 시작 노드를 가리킴.
										for(int j=0;j<(noSortQ->count)-1;j++){ //버블정렬은 1개 적게 하면 된다.
												Node* findN=noSortN; //탐색을 시작하면서 버블정렬을 할 
												for(int k=0; k<(noSortQ->count)-j-1; k++){ //이번엔 j번째 인덱스의 노드부터 버블시작.
														struct stat buf1,buf2;
														lstat(findN->path,&buf1);
														lstat(findN->next->path,&buf2);
														if(buf1.st_uid > buf2.st_uid) //순서가 잘못됐으면..
																swapNdata(findN,findN->next);

														findN=findN->next; //다음버블을 위해 노드 위치 이동.
												}
										}
										noSortQ=noSortQ->next; //다음 세트로 이동.
								}	

						}
						else{

								Queue* noSortQ=r->front; //시작 세트.
								for(int i=0; i<count; i++){ //모든 세트를 돌겠다. 
										Node* noSortN=noSortQ->front; //이번에 정렬할 세트의 시작 노드를 가리킴.
										for(int j=0;j<(noSortQ->count)-1;j++){ //버블정렬은 1개 적게 하면 된다.
												Node* findN=noSortN; //탐색을 시작하면서 버블정렬을 할 
												for(int k=0; k<(noSortQ->count)-j-1; k++){ //이번엔 j번째 인덱스의 노드부터 버블시작.
														struct stat buf1,buf2;
														lstat(findN->path,&buf1);
														lstat(findN->next->path,&buf2);
														if(buf1.st_uid < buf2.st_uid) //순서가 잘못됐으면..
																swapNdata(findN,findN->next);

														findN=findN->next; //다음버블을 위해 노드 위치 이동.
												}
										}
										noSortQ=noSortQ->next; //다음 세트로 이동.
								}

						}

				}
				else if(!strcmp(category,"gid")){

						if(order>0){

								Queue* noSortQ=r->front; //시작 세트.
								for(int i=0; i<count; i++){ //모든 세트를 돌겠다. 
										Node* noSortN=noSortQ->front; //이번에 정렬할 세트의 시작 노드를 가리킴.
										for(int j=0;j<(noSortQ->count)-1;j++){ //버블정렬은 1개 적게 하면 된다.
												Node* findN=noSortN; //탐색을 시작하면서 버블정렬을 할 
												for(int k=0; k<(noSortQ->count)-j-1; k++){ //이번엔 j번째 인덱스의 노드부터 버블시작.
														struct stat buf1,buf2;
														lstat(findN->path,&buf1);
														lstat(findN->next->path,&buf2);
														if(buf1.st_gid > buf2.st_gid) //순서가 잘못됐으면..
																swapNdata(findN,findN->next);

														findN=findN->next; //다음버블을 위해 노드 위치 이동.
												}
										}
										noSortQ=noSortQ->next; //다음 세트로 이동.
								}

						}
						else{

								Queue* noSortQ=r->front; //시작 세트.
								for(int i=0; i<count; i++){ //모든 세트를 돌겠다. 
										Node* noSortN=noSortQ->front; //이번에 정렬할 세트의 시작 노드를 가리킴.
										for(int j=0;j<(noSortQ->count)-1;j++){ //버블정렬은 1개 적게 하면 된다.
												Node* findN=noSortN; //탐색을 시작하면서 버블정렬을 할 
												for(int k=0; k<(noSortQ->count)-j-1; k++){ //이번엔 j번째 인덱스의 노드부터 버블시작.
														struct stat buf1,buf2;
														lstat(findN->path,&buf1);
														lstat(findN->next->path,&buf2);
														if(buf1.st_gid < buf2.st_gid) //순서가 잘못됐으면..
																swapNdata(findN,findN->next);

														findN=findN->next; //다음버블을 위해 노드 위치 이동.
												}
										}
										noSortQ=noSortQ->next; //다음 세트로 이동.
								}

						}

				}
				else { //mode별로

						if(order>0){

								Queue* noSortQ=r->front; //시작 세트.
								for(int i=0; i<count; i++){ //모든 세트를 돌겠다. 
										Node* noSortN=noSortQ->front; //이번에 정렬할 세트의 시작 노드를 가리킴.
										for(int j=0;j<(noSortQ->count)-1;j++){ //버블정렬은 1개 적게 하면 된다.
												Node* findN=noSortN; //탐색을 시작하면서 버블정렬을 할 
												for(int k=0; k<(noSortQ->count)-j-1; k++){ //이번엔 j번째 인덱스의 노드부터 버블시작.
														struct stat buf1,buf2;
														lstat(findN->path,&buf1);
														lstat(findN->next->path,&buf2);
														if(buf1.st_mode > buf2.st_mode) //순서가 잘못됐으면..
																swapNdata(findN,findN->next);

														findN=findN->next; //다음버블을 위해 노드 위치 이동.
												}
										}
										noSortQ=noSortQ->next; //다음 세트로 이동.
								}	

						}
						else{

								Queue* noSortQ=r->front; //시작 세트.
								for(int i=0; i<count; i++){ //모든 세트를 돌겠다. 
										Node* noSortN=noSortQ->front; //이번에 정렬할 세트의 시작 노드를 가리킴.
										for(int j=0;j<(noSortQ->count)-1;j++){ //버블정렬은 1개 적게 하면 된다.
												Node* findN=noSortN; //탐색을 시작하면서 버블정렬을 할 
												for(int k=0; k<(noSortQ->count)-j-1; k++){ //이번엔 j번째 인덱스의 노드부터 버블시작.
														struct stat buf1,buf2;
														lstat(findN->path,&buf1);
														lstat(findN->next->path,&buf2);
														if(buf1.st_mode < buf2.st_mode) //순서가 잘못됐으면..
																swapNdata(findN,findN->next);

														findN=findN->next; //다음버블을 위해 노드 위치 이동.
												}
										}
										noSortQ=noSortQ->next; //다음 세트로 이동.
								}

						}

				}
		}
}

void printroot(Root* r){
		Queue* pq=r->front;
		struct stat buf;
		int i=1;
		char changeSize[100];
		char changeMtime[100];
		char changeAtime[100];
		while(pq!=NULL){
				Node* n=pq->front;
				get_size(pq->size,changeSize);
				printf("---- Identical files #%d (%s bytes - %s) ---- \n",i++,changeSize,pq->md);
				int j=1;
				while(n!=NULL){
						if(lstat(n->path,&buf)==0){
								get_time(buf.st_mtime,changeMtime);
								get_time(buf.st_atime,changeAtime);
								printf("[%d] %s (mtime : %s) (atime : %s) (uid : %d) (gid : %d) (mode : %o)\n",j++,n->path,
												changeMtime,changeAtime,buf.st_uid,buf.st_gid,buf.st_mode);
						}
						n=n->next;
				}
				printf("\n\n");
				pq=pq->next;
		}
}

Queue* queueInSet(Root* r,int setNum){
		Queue* q=r->front; //여기가 실제로 세트 모음들 중 1번째 세트임.
		for(int i=1;i<setNum;i++){
				q=q->next;
		}
		return q;
}

void deleteroot(Root* root){
		Queue* q=root->front; //앞에서부터 삭제 해나가기.
		while(q!=NULL){
				Queue* delq=q; //삭제할 큐 기억
				q=q->next;//다음 큐로 넘어가있음.
				deletequeue(delq); //삭제.
				root->count --;
		}
		free(root);
}

//비었다면 0이 리턴 될 것임.
int isEmptyRoot(Root* root){
		return ((root->count)==0)?1:0;
}


typedef struct TrashNode{
		char* originPath; //
		char* trashPath;
		time_t delete_t;
		struct TrashNode* next; //자기자신을 아직 컴파일러는 Node라는 이름으로 모름..
}TrashNode;

typedef struct TrashRoot{
		TrashNode* front;
		int count;
}TrashRoot;

int make_trash(TrashRoot* root){

		struct dirent **fileList;

		int cnt = 0;

		if ((cnt = scandir(TrashInfoPath, &fileList, NULL, alphasort)) == -1){
				printf("ERROR: scandir error for %s\n",TrashInfoPath );
				return -1;
		}

		for(int i=0;i<cnt;i++){
				if(!strcmp(fileList[i]->d_name,".") || !strcmp(fileList[i]->d_name,"..")){
						free(fileList[i]);
						continue;
				}


				char fullPath[PATHMAX];
				sprintf(fullPath,"%s/%s",TrashInfoPath,fileList[i]->d_name);
				FILE* fp;
				if((fp=fopen(fullPath,"r"))==NULL){
						printf("open_error.....\n");
						free(fileList[i]);
						continue;
				}

				TrashNode* newNode=(TrashNode*)malloc(sizeof(TrashNode));

				char input[10000];
				fgets(input,10000,fp);
				char* ptr = NULL;

				ptr = strtok(input," ");
				newNode->originPath=(char*)malloc(strlen(ptr)+1);
				strcpy(newNode->originPath,ptr);
				newNode->originPath[strlen(ptr)]='\0';

				ptr = strtok(NULL," ");
				newNode->trashPath=(char*)malloc(strlen(ptr)+1);
				strcpy(newNode->trashPath,ptr);	
				newNode->trashPath[strlen(ptr)]='\0';

				ptr = strtok(NULL," ");
				newNode->delete_t=atol(ptr);

				newNode->next=NULL;

				TrashNode* cur=root->front;
				if(root->front==NULL){
						root->front=newNode;	
						root->count++;
				}
				else{
						TrashNode* tmp=root->front; //시작 노드를 가리킴.
						root->front=newNode;
						newNode->next=tmp;
						root->count++;
				}

				fclose(fp);
				free(fileList[i]);
		}

		free(fileList);

		return cnt;	
}


void swapTdata(TrashNode* n1,TrashNode* n2){
		char* tmp_originPath=n1->originPath;
		char* tmp_trashPath=n1->trashPath;
		time_t tmp_time=n1->delete_t;

		n1->originPath=n2->originPath;
		n1->trashPath=n2->trashPath;
		n1->delete_t=n2->delete_t;

		n2->originPath=tmp_originPath;
		n2->trashPath=tmp_trashPath;
		n2->delete_t=tmp_time;
}

void sort_trash(TrashRoot* root,char* category,int order){
		if(!strcmp(category,"filename")){

				if(order>0){ //오름차순	
						TrashNode* noSortN=root->front; //이번에 정렬할 세트의 시작 노드를 가리킴.
						for(int j=0;j<(root->count)-1;j++){ //버블정렬은 1개 적게 하면 된다.
								TrashNode* findN=noSortN; //탐색을 시작하면서 버블정렬을 할 
								for(int k=0; k<(root->count)-j-1; k++){ //이번엔 j번째 인덱스의 노드부터 버블시작.
										if(comparePath(findN->originPath,findN->next->originPath) < 0) //순서가 잘못됐으면..
												swapTdata(findN,findN->next);

										findN=findN->next; //다음버블을 위해 노드 위치 이동.
								}
						}

				}
				else{ 

						TrashNode* noSortN=root->front; //이번에 정렬할 세트의 시작 노드를 가리킴.
						for(int j=0;j<(root->count)-1;j++){ //버블정렬은 1개 적게 하면 된다.
								TrashNode* findN=noSortN; //탐색을 시작하면서 버블정렬을 할 
								for(int k=0; k<(root->count)-j-1; k++){ //이번엔 j번째 인덱스의 노드부터 버블시작.
										if(comparePath(findN->originPath,findN->next->originPath) > 0) //순서가 잘못됐으면..
												swapTdata(findN,findN->next);

										findN=findN->next; //다음버블을 위해 노드 위치 이동.
								}
						}

				}
		}
		else if(!strcmp(category,"size")){

				if(order>0){ //오름차순	
						TrashNode* noSortN=root->front; //이번에 정렬할 세트의 시작 노드를 가리킴.
						for(int j=0;j<(root->count)-1;j++){ //버블정렬은 1개 적게 하면 된다.
								TrashNode* findN=noSortN; //탐색을 시작하면서 버블정렬을 할 
								for(int k=0; k<(root->count)-j-1; k++){ //이번엔 j번째 인덱스의 노드부터 버블시작.
										struct stat buf1,buf2;
										lstat(findN->trashPath,&buf1);
										lstat(findN->next->trashPath,&buf2);
										if(buf1.st_size > buf2.st_size) //오름차순이 아니라면.. 잘못됐으면..
												swapTdata(findN,findN->next);

										findN=findN->next; //다음버블을 위해 노드 위치 이동.
								}
						}

				}
				else{ 

						TrashNode* noSortN=root->front; //이번에 정렬할 세트의 시작 노드를 가리킴.
						for(int j=0;j<(root->count)-1;j++){ //버블정렬은 1개 적게 하면 된다.
								TrashNode* findN=noSortN; //탐색을 시작하면서 버블정렬을 할 
								for(int k=0; k<(root->count)-j-1; k++){ //이번엔 j번째 인덱스의 노드부터 버블시작.
										struct stat buf1,buf2;
										lstat(findN->trashPath,&buf1);
										lstat(findN->next->trashPath,&buf2);
										if(buf1.st_size < buf2.st_size) //내림차순이 아니라면.. 잘못됐으면..
												swapTdata(findN,findN->next);

										findN=findN->next; //다음버블을 위해 노드 위치 이동.
								}
						}

				}
		}
		else if(!strcmp(category,"date")){

				if(order>0){
						TrashNode* noSortN=root->front; //이번에 정렬할 세트의 시작 노드를 가리킴.
						for(int j=0;j<(root->count)-1;j++){ //버블정렬은 1개 적게 하면 된다.
								TrashNode* findN=noSortN; //탐색을 시작하면서 버블정렬을 할 
								for(int k=0; k<(root->count)-j-1; k++){ //이번엔 j번째 인덱스의 노드부터 버블시작.
										if((findN->delete_t) > (findN->next->delete_t)) //오름차순이 아니라면.. 잘못됐으면..
												swapTdata(findN,findN->next);

										findN=findN->next; //다음버블을 위해 노드 위치 이동.
								}
						}
				}
				else{

						TrashNode* noSortN=root->front; //이번에 정렬할 세트의 시작 노드를 가리킴.
						for(int j=0;j<(root->count)-1;j++){ //버블정렬은 1개 적게 하면 된다.
								TrashNode* findN=noSortN; //탐색을 시작하면서 버블정렬을 할 
								for(int k=0; k<(root->count)-j-1; k++){ //이번엔 j번째 인덱스의 노드부터 버블시작.
										if((findN->delete_t) < (findN->next->delete_t)) //내림차순이 아니라면.. 잘못됐으면..
												swapTdata(findN,findN->next);

										findN=findN->next; //다음버블을 위해 노드 위치 이동.
								}
						}

				}

		}
		else if(!strcmp(category,"time")){
				if(order>0){ //오름차순

						struct tm* t1;
						struct tm* t2;

						TrashNode* noSortN=root->front; //이번에 정렬할 세트의 시작 노드를 가리킴.
						for(int j=0;j<(root->count)-1;j++){ //버블정렬은 1개 적게 하면 된다.
								TrashNode* findN=noSortN; //탐색을 시작하면서 버블정렬을 할 
								for(int k=0; k<(root->count)-j-1; k++){ //이번엔 j번째 인덱스의 노드부터 버블시작.
										t1=localtime(&(findN->delete_t));
										int t1_hour=t1->tm_hour;
										int t1_min=t1->tm_min;
										int t1_sec=t1->tm_sec;

										t2=localtime(&(findN->next->delete_t));
										int t2_hour=t2->tm_hour;
										int t2_min=t2->tm_min;
										int t2_sec=t2->tm_sec;

										if(t1_hour > t2_hour){ //오름차순이 아니라면.. 잘못됐으면..
												swapTdata(findN,findN->next);
										}
										else if(t1_hour==t2_hour){
												if(t1_min > t2_min){
														swapTdata(findN,findN->next);
												}
												else if(t1_min == t2_min){
														if(t1_sec > t2_sec){
																swapTdata(findN,findN->next);
														}
												}
										}

										findN=findN->next; //다음버블을 위해 노드 위치 이동.
								}
						}

				}
				else{

						struct tm* t1;
						struct tm* t2;

						TrashNode* noSortN=root->front; //이번에 정렬할 세트의 시작 노드를 가리킴.
						for(int j=0;j<(root->count)-1;j++){ //버블정렬은 1개 적게 하면 된다.
								TrashNode* findN=noSortN; //탐색을 시작하면서 버블정렬을 할 
								for(int k=0; k<(root->count)-j-1; k++){ //이번엔 j번째 인덱스의 노드부터 버블시작.
										t1=localtime(&(findN->delete_t));
										int t1_hour=t1->tm_hour;
										int t1_min=t1->tm_min;
										int t1_sec=t1->tm_sec;

										t2=localtime(&(findN->next->delete_t));
										int t2_hour=t2->tm_hour;
										int t2_min=t2->tm_min;
										int t2_sec=t2->tm_sec;

										if(t1_hour < t2_hour){ //오름차순이 아니라면.. 잘못됐으면..
												swapTdata(findN,findN->next);
										}
										else if(t1_hour==t2_hour){
												if(t1_min < t2_min){
														swapTdata(findN,findN->next);
												}
												else if(t1_min == t2_min){
														if(t1_sec < t2_sec){
																swapTdata(findN,findN->next);
														}
												}
										}

										findN=findN->next; //다음버블을 위해 노드 위치 이동.
								}
						}
				}
		}
}

void print_trash(TrashRoot* root){
		if(root->front==NULL){
			return;
		}
		printf("%s\t\t%s\t\t%s\t\t%s\n","FILENAME","SIZE","DELETION DATE","DELETION_TIME");
		int i=1;

		TrashNode* tmp=root->front;
		while(tmp!=NULL){
				struct stat statbuf;
				lstat(tmp->trashPath,&statbuf);
				struct tm* t=localtime(&(tmp->delete_t));
				printf("[%d] %s\t\t%ld\t\t%d-%d-%d\t\t%d:%d:%d\n",i,tmp->originPath,statbuf.st_size,
								t->tm_year+1900, t->tm_mon+1, t->tm_mday,
								t->tm_hour, t->tm_min, t->tm_sec);
				tmp=tmp->next;
				i++;
		}

}



