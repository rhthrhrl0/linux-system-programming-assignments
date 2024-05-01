
off_t returnByteSize(char* input,int* errorSize){
	*errorSize=0; //초기화
	off_t frontInt=0;
	off_t backFloat=0;
	char back[10]="000000000";
	
	int isDot=0; 
	int KB=0;
	int MB=0;
	int GB=0;
	//int floatDigit=0;
	int floatNum=0; 
	off_t length=strlen(input);
	for(int i=0;i<length;i++){
		if(((int)input[i])>=48 && ((int)input[i]) <=57 && isDot==0 ) //소수점이 나오기 전까지의 숫자들
				frontInt=frontInt*10+((int)input[i])-48;
		else if(((int)input[i])>=48 && ((int)input[i]) <=57 && isDot==1){ //소수점이 나온 이후
				if(floatNum==9) //9자리이상은 받을 필요 없음.
						continue;
				back[floatNum++]=input[i];
		}
		else if(input[i]=='.'){
				if(isDot==1){ //소수점이 한번 나온적이 이미 있었으면 에러임.
					*errorSize=1;
					return 0;
				}
				isDot=1;
		}
		else{
				if(length-1==i+1){ //숫자나 소수점 이외의 문자는 나와도 뒤에서 두번째에서 나와야함.
					if( (strstr(input+i-1,"KB")!=NULL) || (strstr(input+i-1,"kb")!=NULL)){
								KB=1;
								break;
					}
					else if( (strstr(input+i,"MB")!=NULL) || (strstr(input+i,"mb")!=NULL)){
								MB=1;
								break;
					}
					else if( (strstr(input+i,"GB")!=NULL) || (strstr(input+i,"gb")!=NULL) ){
								GB=1;
								break;
					}
					else{
							*errorSize=1;
							return 0;
					}
			
				}
				else{
					*errorSize=1;
					return 0;
				}
		}
	}
	
	if(KB)
		if(isDot){
			for(int i=0;i<3;i++){
				backFloat=backFloat*10+(int)back[i]-48;
			}
			return frontInt*1000+backFloat;
		}
		else
			return frontInt*1000;
	else if(MB)
		if(isDot){
			for(int i=0;i<6;i++)
				backFloat=backFloat*10+(int)back[i]-48;
			return frontInt*1000000+backFloat;
		}
		else
			return frontInt*1000000;	
	else if(GB)
		if(isDot){
			for(int i=0;i<9;i++)
				backFloat=backFloat*10+(int)back[i]-48;
			return frontInt*1000000000+backFloat;
		}
		else
			return frontInt*1000000000;		
	
	else
		return frontInt;
	
}


