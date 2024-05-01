#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/stat.h>
#include<fcntl.h>

int main(){
	char* line1="Usage:\n";
	char* line2="> fmd5/fsha1 [FILE_EXTENSION] [MINSIZE] [MAXSIZE] [TARGET_DIRECTORY]\n";
	char* line3=">> [SET_INDEX] [OPTION ... ]\n";
	char* line4="[OPTION ... ]\n";
	char* line5="d [LIST_IDX] : delete [LIST_IDX] file\n";
	char* line6="i : ask for confirmation before delete\n";
	char* line7="f : force delete except the recently modified file\n";
	char* line8="t : force move to Trash except the recently modified file\n";
	char* line9="> help\n";
	char* line10="> exit\n";

	printf("%s",line1);
	printf("%-1s%s","",line2);
	printf("%-5s%s","",line3);
	printf("%-8s%s","",line4);
	printf("%-8s%s","",line5);
	printf("%-8s%s","",line6);
	printf("%-8s%s","",line7);
	printf("%-8s%s","",line8);
	printf("%-1s%s","",line9);
	printf("%-1s%s","",line10);
	printf("\n\n");
	exit(0);
}
