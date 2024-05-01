typedef struct Node{
	char* path;
	struct Node* next; //자기자신을 아직 컴파일러는 Node라는 이름으로 모름..
}Node;

typedef struct Queue{
	Node* front; 
	Node* rear;
	int count; //큐의 노드 개수
	off_t size; //해당 파일의 크기.
	unsigned char sha[SHA_DIGEST_LENGTH]; //해당 큐가 담당하는 해쉬값.
	struct Queue* next; //Root구조체에서 쓸것임. 큐는 자신의 next멤버를 직접  안씀.
}Queue;

typedef struct SizeNode{
	off_t size; //해당 큐가 담당하는 해쉬값.
	struct SizeNode* next;
	char count; //1개이면 '1'이고 그이상은 '2'로 표시.
}SizeNode;

typedef struct SizeQueue{
	SizeNode* front;
	SizeNode* rear;
	int count;
}SizeQueue;

typedef struct Root{
	Queue* front;
	Queue* rear;
	int count; 
}Root;

void get_time(time_t stime,char* recieveTime){
	char* time = (char*)malloc(sizeof(char) * BUFMAX);
	struct tm *tm;
	tm = localtime(&stime);
	sprintf(time, "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	strcpy(recieveTime,time);
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

void initqueue(Queue* q,off_t fSize,unsigned char* sha1){
	q->front=NULL;
	q->rear=NULL;
	q->count=0; //큐에 있는 노드의 수.
	q->size=fSize; //해당 큐에 들어갈 파일들의 크기
	memcpy(q->sha, sha1, SHA_DIGEST_LENGTH); //해쉬값 복사. 해당 큐가 맡을 해쉬의 정보.
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

void dequeue(Queue* q,char* returnPath){ 
	if(q->count==0) 
			returnPath=NULL; //큐가 비었으면 돌려줄게 없음
	else{
			strcpy(returnPath,q->front->path);
			Node* n=q->front; //임시로 현재의 front를 받아줄 놈
			q->front=q->front->next; //front는 다음 노드로 넘어감
			free(n->path);
			free(n); //프리해서 해제
			q->count=(q->count)-1;
	}
}

void deletequeue(Queue* q){
	while(q->count!=0){
		Node* n=q->front; //임시로 현재의 front를 받아줄 놈
		q->front=q->front->next; //front는 다음 노드로 넘어감
		free(n->path);
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

void enroot(Root* r,SizeQueue* sq,char* path,off_t fSize,unsigned char* sha1){
	int isFindInSQ=0; 
	SizeNode* sn=sq->front;
	while(sn!=NULL){
		isFindInSQ=1; 

		if(sn->size!=fSize)
			isFindInSQ=0;
		
		if(isFindInSQ){ //while문을 나갈때 1을 유지한다면....얘는 중복이 있는 파일임.
				break;
		}

		sn=sn->next;
	}
	
	//중복해쉬값을 저장한 해쉬큐에 있는 해쉬값들 중 같은게 없음.
	if(isFindInSQ==0){
		return; //종료. 삽입안할거임.
	}
	
	if(r->count==0){
		Queue* newQ=(Queue*)malloc(sizeof(Queue));
		initqueue(newQ,fSize,sha1); //큐초기화
		enqueue(newQ,path); //큐에 삽입.
		r->front=newQ;
		r->rear=newQ;
		r->count ++;
	}
	else{
		Queue* indexQ=r->front;
		int findQueue=0;
		for(int i=0; i < r->count ; i++){
			int same=1;
			for(int j=0;j<SHA_DIGEST_LENGTH;j++)
					if(indexQ->sha[j]!=sha1[j]){
						same=0;
						break;
					}

			if(same){
				findQueue=1; //큐를 찾았으면 1로
				enqueue(indexQ,path);
				break;
			}

			indexQ=indexQ->next;
		}
		//큐를 못찾음... 새로 루트 꼬리에 새로운 큐 넣어주기.
		if(findQueue==0){
			Queue* newQ=(Queue*)malloc(sizeof(Queue));
			initqueue(newQ,fSize,sha1); //큐초기화
			enqueue(newQ,path); //해당큐에 삽입.
			r->rear->next=newQ;
			r->rear=newQ;
			r->count ++;
		}
	}

}

//수정할 세트인덱스와 옵션상태,삭제할인덱스들의 정보
void deroot(Root* r,int setNum,char option,int* deleteIndex){
	
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
					free(freeN->path);
					free(freeN);
					n=q->front; //n한칸 전진.
				}
				else{
					freeN=n;
					previousN->next=n->next; //n제거하기 전에 이어주기.
					printf("\"%s\" has been deleted in #%d\n",n->path,setNum);
					unlink(n->path); //삭제.
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
					free(freeN->path);
					free(freeN);
					n=q->front; //n한칸 전진.
				}
				else{
					freeN=n;
					previousN->next=n->next; //n제거하기 전에 이어주기.
					unlink(n->path);
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
		if(q->count==0){
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
			char* TMPpath=n->path;
			n->path=remainNode->path; //맨앞으로 교체.
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
			free(freeN->path);
			free(freeN); //해제
		}
		q->count=1;
		char changeMtime[100];
		get_time(max,changeMtime);
		printf("Left file in #%d : \"%s\" (%s)\n",setNum,q->front->path,changeMtime);
		
	}
	else if(option=='t'){ //쓰레기통으로 보내야함.....
		char TrashPath[PATHMAX];
		realpath("Trash",TrashPath);
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
			char* TMPpath=n->path;
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
			if((numInDir=scandir(TrashPath,&fileList,NULL,alphasort))==-1){ //파일리스트 불러옴.
				fprintf(stderr,"error: 디렉토리를 열 수 없습니다.\n");
				return;
			}

			freeN=n; //삭제할건 얘가 기억.
			n=n->next; //일단 넘어감.
			char filename[257];
			char newPath[PATHMAX];
			char newFileName[257];
		
			findNameFunc(freeN->path,filename);
			strcpy(newFileName,filename);
			
			int nameNum=0;
			int isNameSame=0;
			while(1){
				isNameSame=0;
				for(int i=0;i<numInDir;i++){ //기존 쓰레기통에 같은 이름인 파일이있다면...
					if(strcmp(fileList[i]->d_name,newFileName)==0){
							sprintf(newFileName,"%s(%d)",filename,++nameNum);
							isNameSame=1;
							break;
					}
				}
				if(isNameSame==0)
					break;
			}
			
			if(nameNum==0){
				sprintf(newPath,"%s/%s",TrashPath,filename);
			}
			else{
				sprintf(newPath,"%s/%s",TrashPath,newFileName);
			}

			if(link(freeN->path,newPath)<0)
					printf("can't move in Trash....\n");
			unlink(freeN->path); //삭제....원래 쓰레기통으로 보내야함.
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

}

//큐에 있는 노드 개수가 1인 큐를  모두 삭제하는 메소드.
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
}

void swqpQdata(Queue* q1,Queue* q2){

	//next빼고 다 바꿈.
	Node* TMPfront=q1->front;
	Node* TMPrear=q1->rear;
	int TMPcount=q1->count;
	off_t TMPsize=q1->size;
	unsigned char TMPsha[SHA_DIGEST_LENGTH];
	memcpy((unsigned char*)TMPsha,(unsigned char*)(q1->sha),(size_t)SHA_DIGEST_LENGTH);

	q1->front=q2->front;
	q1->rear=q2->rear;
	q1->count=q2->count;
	q1->size=q2->size;
	memcpy((unsigned char*)(q1->sha),(unsigned char*)(q2->sha),(size_t)SHA_DIGEST_LENGTH);

	q2->front=TMPfront;
	q2->rear=TMPrear;
	q2->count=TMPcount;
	q2->size=TMPsize;
	memcpy((unsigned char*)(q2->sha),(unsigned char*)(TMPsha),(size_t)SHA_DIGEST_LENGTH);
}

void sortroot(Root* r){

	int count=r->count;
	for(int i=0;i < count-1;i++){
		Queue* noSortQ=r->front;
		for(int j=0;j<count-i-1;j++){
			if(noSortQ->size > noSortQ->next->size){
				swqpQdata(noSortQ,noSortQ->next);
			}
			noSortQ=noSortQ->next;
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
		printf("---- Identical files #%d (%s bytes - ",i++,changeSize);
		for(int a=0;a<SHA_DIGEST_LENGTH;a++){
			printf("%02x",pq->sha[a]);
		}
		printf(") ----\n");
		int j=1;
		while(n!=NULL){
			if(lstat(n->path,&buf)==0){
				get_time(buf.st_mtime,changeMtime);
				get_time(buf.st_atime,changeAtime);
				printf("[%d] %s (mtime : %s) (atime : %s)\n",j++,n->path,
								changeMtime,changeAtime);
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




void initqueueSize(SizeQueue* s){
	s->front=NULL;
	s->rear=NULL;
	s->count=0; //큐에 있는 노드의 수.
}

void enqueueSize(SizeQueue* s,off_t fSize){
	if(s->count==0){
		SizeNode* newN=(SizeNode*)malloc(sizeof(SizeNode));
		newN->size=fSize;
		newN->count='1';
		newN->next=NULL;
		s->front=newN;
		s->rear=newN;
		s->count ++;
	}
	else{
		SizeNode* indexN=s->front;
		int findQueue=0;
		for(int i=0; i < s->count ; i++){
			int same=1;
			if(indexN->size!=fSize){
				same=0;
			}

			if(same){
				findQueue=1; //큐를 찾았으면 1로
				indexN->count='2'; //해당 해쉬값을 가진 파일이 더 있음을 표시.
				break;
			}

			indexN=indexN->next;
		}
		//노드를 못찾음... 새로운 노드 넣어주기.
		if(findQueue==0){
			SizeNode* newN=(SizeNode*)malloc(sizeof(SizeNode));
			newN->size=fSize;
			newN->count='1';
			newN->next=NULL;
			
			s->rear->next=newN;
			s->rear=newN;
			s->count ++;
		}
	}	
}


//해쉬 큐 삭제.
void deletequeueSize(SizeQueue* s){
	while(s->front!=NULL){
		SizeNode* n=s->front; //임시로 현재의 front를 받아줄 놈
		s->front=s->front->next; //front는 다음 노드로 넘어감
		free(n); //프리해서 해제
		s->count=(s->count)-1;
	}
	free(s);
}


//해쉬큐에 있는 중복없는 노드를 모두 삭제하는 메소드.
void cleanqueueSize(SizeQueue* s){
	SizeNode* n=s->front;
	SizeNode* previousN=NULL;
	SizeNode* freeN=NULL;
	int del=0;
	while(n!=NULL){
		if(n->count=='1'){
			del++;
			if(previousN==NULL){
				freeN=n;
				n=n->next;
				s->front=n;
				free(freeN);
			}
			else{
				freeN=n;
				n=n->next;
				previousN->next=n;
				free(freeN);
			}
			s->count--;
		}
		else{
			previousN=n;
			n=n->next;
		}
	}
}
