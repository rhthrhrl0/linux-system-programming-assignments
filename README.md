# linux-system-programming-assignments
리눅스 시스템 프로그래밍 수업 과목 과제 코드 및 보고서 정리

# 과제1 ssu_sindex
지정한 파일 또는 디렉토리의 이름을 탐색해서 파일 및 디렉토리의 속성을 비교하는 프로그램.

프로그램이 실행되고, 프롬프트가 떠야 함.(예시: 학번>> )
이 프롬프트 상에서 입력을 받으며, find, help, exit 명령어를 입력할 수 있다. 여기서는 find에 대해서만 설명하며 자세한 것은 보고서에 나와있음. 

find 명령어의 첫번째 인자는 기준 파일명(혹은 디렉토리도 가능함. 이는 뒤에 설명),두 번째 인자는 탐색을 시작할 디렉토리 경로임. 
그 이후부터는 옵션들과 이 기준 파일과 이름과 크기가 동일한 모든 파일들을 찾아서, 정보들을 보여줘야 함.

또한, 이렇게 찾은 파일들 중 파일 내용이 기준 파일과 다른 것이 있을 수 있기 때문에 사용자가 지정한 옵션에 따라 파일 내용을 비교해서 기준파일과 다른 점을 출력해서 보여줘야 한다.
-> LCS 알고리즘 활용

만약,입력 받은 첫번째 인자가 디렉토리였다면 이 프로그램의 동작방식은 달라진다. 첫 번째 인자가 디렉토리인 경우는 재귀를 활용해서 그 내부 모든 파일의 사이즈 합을 구하고, 이것을 해당 디렉토리의 크기로 정의한다.
이 경우, 기준 디렉토리와 크기 및 이름이 같은 모든 디렉토리를 찾아서 출력해서 보여주면 된다.
이후, 파일비교때와 동일하게 인덱스를 사용자가 선택해서 두 디렉토리 간의 차이를 보여줄 수 있어야 한다. 이때, 두 디렉토리 간의 차이는 내부에 들어있는 파일들을 기준으로 한다.
한쪽 디렉토리에만 있는 파일이면 한쪽에만 있다고 표시하고, 둘이 이름과 크기가 같은 파일이 있는 경우 모두 파일 비교때처럼 두 파일 간 내용 차이점을 출력한다.

### 학습한 것
1. 디렉토리를 탐색하는 방법
2. 유닉스 파일 시스템의 기본적인 구조와 i-node블록과 데이터 블록의 관계
3. 파일의 정보를 얻을 수 있는 stat 자료구조와 stat 함수 사용법
4. 정규 파일인지 디렉토리 파일인지, 그 외 파일인지를 알 수 있는 stat.st_mode 활용법

# 과제2 

# 과제3

