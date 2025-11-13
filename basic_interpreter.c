/* Basic Interpreter by H?eyin Uslu raistlinthewiz@hotmail.com */
/* Code licenced under GPL */

#include <stdio.h> // 표준 입출력 함수를 사용하기 위한 헤더 파일을 포함합니다.
#include <string.h> // 문자열 조작 함수를 사용하기 위한 헤더 파일을 포함합니다.
#include <stdlib.h> // 일반 유틸리티 함수를 사용하기 위한 헤더 파일을 포함합니다.
#include <ctype.h> // 문자 분류 함수를 사용하기 위한 헤더 파일을 포함합니다.

#ifdef _WIN32 // Windows 환경에서 컴파일할 때 아래 코드를 사용합니다.
#define CLEAR() system("cls") // 화면을 지우는 CREAR() 매크로를 cls 명령어로 정의합니다.
#else // windows가 아닌 다른 환경(MacOS, Linux)에서 컴파일할 때 아래 코드를 사용합니다.
#define CLEAR() system("clear") // 화면을 지우는 CLEAR() 매크로를 clear 명령어로 정의합니다.
#endif // #ifdef 블록의 마지막 입니다.

struct node { // 인터프리터가 코드를 실행하며 상태를 저장하는데 사용하는 주 스택입니다.
    int type; /* 1 var, 2 function, 3 function call, 4 begin, 5 end */
              // 노드의 유형을 저장합니다. (변수(var), 함수 정의(function), 함수 호출 (function call) 등)
    char exp_data; // 변수의 이름 또는 함수의 이름을 저장합니다.
    int val; // 변수의 값 또는 함수와 관련된 임시 값 저장합니다.
    int line; // 함수가 정의된 코드 라인 번호를 저장합니다.
    struct node* next; // 스택의 다음 노드를 가리키는 포인터입니다.
}; // node 구조체 정의의 마지막 입니다.
typedef struct node Node; // struct node를 간단히 'Node'로 사용할 수 있도록 별칭을 정의합니다.

struct stack { Node* top; }; // 주 stack 구조체로 스택의 가장 위(top) 노드를 가리키는 포인터를 포함합니다.
typedef struct stack Stack; // struct stack에 'STACK'이라는 별칭을 정의합니다.

struct opnode { char op; struct opnode* next; }; // MathStack에 사용되는 노드 구조체. 연산자(op)와 다음 노드 포인터를 가집니다.
typedef struct opnode opNode; // struct opnode에 'opNode'라는 별칭을 정의합니다.

struct opstack { opNode* top; }; // MathStack 구조체입니다.
typedef struct opstack OpStack; // struct opstack에 'OpStack'이라는 별칭을 정의합니다.

struct postfixnode { int val; struct postfixnode* next; }; // CalcStack에 사용되는 구조체. 피연산자(val)와 다음 노드 포인터를 가집니다.
typedef struct postfixnode Postfixnode; // struct postfixnode에 'Postfixnode'라는 별칭을 정의합니다.

struct postfixstack { Postfixnode* top; }; // CalcStack 구조체입니다.
typedef struct postfixstack PostfixStack; // struct postfixstack에 'PostfixStack'이라는 별칭을 정의합니다.

static int GetVal(char, int*, Stack*); // 변수/함수의 값 또는 라인 정보를 스택에서 찾는 정적 함수의 선언입니다.
static int GetLastFunctionCall(Stack*); // 가장 최근 함수 호출이 발생한 라인 번호를 스택에서 찾는 정적 함수의 선언입니다.
static Stack* FreeAll(Stack*); // 메인 스택(STACK)의 모든 노드 메모리를 해제하는 정적 함수의 선언입니다.
static int my_stricmp(const char* a, const char* b); // 대소문자를 구분하지 않고 문자열을 비교하는 정적 함수의 선언입니다.
static void rstrip(char* s); // 문자열 끝의 공백, 개행 문자를 제거하는 정적 함수의 선언입니다.

static Stack* Push(Node sNode, Stack* stck) // 주 스택(STACK)에 새 'Node'를 추가하는 Push 함수입니다.
{
    Node* newnode = (Node*)malloc(sizeof(Node)); // 새 노드를 위한 메모리를 동적으로 할당합니다.
    if (!newnode) { printf("ERROR, Couldn't allocate memory..."); return NULL; } // 메모리 할당 실패 시 에러 메시지를 출력하고 NULL을 반환합니다.
    newnode->type = sNode.type; // 인자로 받은 노드의 유형을 복사합니다.
    newnode->val = sNode.val; // 값을 복사합니다.
    newnode->exp_data = sNode.exp_data; // 이름/데이터를 복사합니다.
    newnode->line = sNode.line; // 라인 번호를 복사합니다.
    newnode->next = stck->top; // 새 노드의 next를 현재 스택의 최상위 노드를 설정합니다.
    stck->top = newnode; // 스택의 최상위 노드 포인터를 새 노드로 업데이트합니다.
    return stck; // 업데이트된 스택 포인터를 반환합니다.
} // Push 함수의 마지막입니다.

static OpStack* PushOp(char op, OpStack* opstck) // MathStack에 연산자(op)를 추가하는 Push 함수입니다.
{
    opNode* newnode = (opNode*)malloc(sizeof(opNode)); // 새 opNode를 위한 메모리를 동적으로 할당합니다.
    if (!newnode) { printf("ERROR, Couldn't allocate memory..."); return NULL; } // 메모리 할당 실패 시 처리입니다.
    newnode->op = op; // 연산자를 저장합니다.
    newnode->next = opstck->top; // 새 노드를 현재 스택의 최상위 노드와 연결합니다.
    opstck->top = newnode; // 스택의 최상위 노드 포인터를 새 노드로 업데이트합니다.
    return opstck; // 업데이트된 스택 포인터를 반환합니다.
} // PushOp 함수의 마지막입니다.

static char PopOp(OpStack* opstck) // 연산자 스택(MathStack)에서 연산자를 제거하고 반환하는 함수입니다.
{
    opNode* temp; // 임시 포인터 변수를 선언합니다.
    char op; // 반환할 연산자를 저장할 변수입니다.
    if (opstck->top == NULL) // 스택이 비어 있는지 확인합니다.
    {
        return 0; // 비어 있다면 0을 반환합니다.
    }
    op = opstck->top->op; // 최상위 노드의 연산자를 저장합니다.
    temp = opstck->top; // 최상위 노드의 포인터를 임시 저장합니다.
    opstck->top = opstck->top->next; // 스택의 최상위 포인터를 다음 노드로 이동시킵니다.
    free(temp); // 이전 최상위 노드의 메모리를 해제합니다.
    return op; // 제거한 연산자를 반환합니다.
} // PopOp 함수의 마지막입니다.

static PostfixStack* PushPostfix(int val, PostfixStack* poststck) // 계산 스택(CalcStack)에 값을 추가하는 Push 함수입니다.
{
    Postfixnode* newnode = (Postfixnode*)malloc(sizeof(Postfixnode)); // 새 Postfixnode를 위한 메모리를 동적으로 할당합니다.
    if (!newnode) { printf("ERROR, Couldn't allocate memory..."); return NULL; } // 메모리 할당 실패 시 처리입니다.
    newnode->val = val; // 값을 저장합니다.
    newnode->next = poststck->top; // 새 노드를 현재 스택의 최상위 노드와 연결합니다.
    poststck->top = newnode;// 스택의 최상위 노드 포인터를 새 노드로 업데이트합니다.
    return poststck; // 업데이트된 스택 포인터를 반환합니다.
} // PushPostfix 함수의 마지막입니다.

static int PopPostfix(PostfixStack* poststck) // 계산 스택(CalcStack)에서 값을 제거하고 반환하는 함수입니다.
{
    Postfixnode* temp; // 임시 포인터 변수를 선언합니다.
    int val; // 반환할 값을 저장할 변수입니다.
    if (poststck->top == NULL) // 스택이 비어있는지 확인합니다.
    {
        return 0; // 비어 있다면 0을 반환합니다.
    }
    val = poststck->top->val; // 최상위 노드의 값을 저장합니다.
    temp = poststck->top; // 최상위 노드의 포인터를 임시 저장합니다.
    poststck->top = poststck->top->next; // 스택의 최상위 포인터를 다음 노드로 이동시킵니다.
    free(temp); // 이전 최상위 노드의 메모리를 해제합니다.
    return val; // 제거한 값을 반환합니다.
} // PopPostfix 함수의 마지막입니다.

static void Pop(Node* sNode, Stack* stck) // 주 스택(STACK)에서 노드를 제거하고 내용을 인자 sNode에 복사하는 함수입니다.
{
    Node* temp; // 임시 포인터 변수를 선언합니다.
    if (stck->top == NULL) return; // 스택이 비어 있다면 아무것도 하지 않고 종료합니다.
    sNode->exp_data = stck->top->exp_data; // 스택 최상위 노드의 데이터(변수 이름, 함수 이름 첫 글자)를 sNode에 복사합니다.
    sNode->type = stck->top->type; // 스택 최상위 노드의 유형(변수, 함수, 호출 등)을 sNode에 복사합니다.
    sNode->line = stck->top->line; // 스택 최상위 노드의 라인 번호(함수 정의 라인, 호출 라인)를 sNode에 복사합니다.
    sNode->val = stck->top->val; // 스택 최상위 노드의 값(변수 값, 반환 값)을 sNode에 복사합니다.
    temp = stck->top; // 최상위 노드의 포인터를 임시 저장합니다.
    stck->top = stck->top->next; // 스택의 최상위 포인터를 다음 노드로 이동시킵니다.
    free(temp); // 이전 최상위 노드의 메모리를 해제합니다.
} // Pop 함수의 마지막입니다.

static int isStackEmpty(OpStack* stck) // 연산자 스택이 비어 있는지 확인하는 함수입니다.
{
    return stck->top == 0; // top이 0이면 비어있다는걸 뜻합니다.
} // isStackEmpty 함수의 마지막입니다.

static int Priotry(char operator) // 주어진 연산자의 우선순위를 반환하는 함수입니다.
{
    if ((operator=='+') || (operator=='-')) return 1; // 연산자가 '+' 또는 '-'일 경우 우선순위 레벨 1을 반환합니다.
    else if ((operator=='/') || (operator=='*')) return 2; // 연산자가 '*' 또는 '/'일 경우 우선순위 레벨 2를 반환합니다.
    return 0; //정의된 연산자가 아닌 경우 우선순위 레벨 0을 반환합니다.
}

int main(int argc, char** argv) // 프로그램의 메인 함수입니다.
{
    char line[4096]; // 파일에서 읽은 현재 라인을 저장하는 버퍼입니다.
    char dummy[4096]; // 파일 포인터를 이동시키기 위해 임시로 라인을 읽는 데 사용되는 더미 버퍼입니다.
    char lineyedek[4096]; // 원본 라인을 저장하는 백업 버퍼입니다.
    char postfix[4096]; // 중위 표현식을 변환한 후위 표현식을 저장하는 버퍼입니다.
    char* firstword; // 라인에서 분리된 첫 번째 단어를 가리키는 포인터입니다.

    int val1; // 후위 표기식 계산 시 사용되는 첫 번째 피연산자입니다.
    int val2; // 후위 표기식 계산 시 사용되는 두 번째 피연산자입니다.

    int LastExpReturn = 0; // 가장 최근에 계산된 표현식의 결과 값을 저장합니다.
    int LastFunctionReturn = -999; // 함수 호출 후 반환된 값을 임시로 저장합니다.
    int CalingFunctionArgVal = 0; // 함수 호출 시 전달되는 인자 값을 임시로 저장합니다.
 
    Node tempNode; // 스택에 Push하기 전에 데이터를 저장하거나 Pop된 데이터를 받는 임시 노드 구조체입니다.

    OpStack* MathStack = (OpStack*)malloc(sizeof(OpStack)); // 연산자 저장에 사용되는 스택 구조체에 메모리를 할당합니다.
    FILE* filePtr; // 입력 소스 파일을 가리키는 포인터입니다.
    PostfixStack* CalcStack = (PostfixStack*)malloc(sizeof(PostfixStack)); // 후위 표기식 계산 시 피연산자를 저장하는 스택 구조체에 메로리를 할당합니다.
    int resultVal = 0; // 후위 표기식 계산 중 중간 결과 값을 저장합니다.
    Stack* STACK = (Stack*)malloc(sizeof(Stack)); // 변수, 함수, 블록 등의 정보를 저장하는 메인 스택 구조체에 메모리를 할당합니다.

    int curLine = 0; // 파일에서 읽고 있는 현재 라인 번호를 추적합니다.
    int foundMain = 0; // main함수를 찾았는지 여부를 나타내는 플래그입니다. (찾음 : 1, 못 찾음 : 0)
    int WillBreak = 0; // 함수 호출이 발생했을 때 현재 라인 처리를 중단하고 파일 포인터를 이동해야 함을 나타내는 플래그입니다.

    if (!MathStack || !CalcStack || !STACK) { // MathStack, CalcStack, STACK 중 하나라도 메모리 할당에 실패하여 NULL이 반환되었는지 검사합니다.
        printf("Memory alloc failed\n"); // 할당 실패 시 에러 메시지를 출력합니다.
        return 1; // 에러코드 1로 종료합니다.
    }
    MathStack->top = NULL; // MathStack의 top을 NULL로 초기화합니다.
    CalcStack->top = NULL; // CalcStack의 top을 NULL로 초기화합니다.
    STACK->top = NULL; // STACK의 top을 NULL로 초기화합니다.

    CLEAR(); // 위에서 정의된 매크로를 이용해 콘솔 화면을 지웁니다.

    if (argc != 2) // 프로그램 실행 시 인자의 개수가 2개가 아닌 경우를 확인합니다.
    {
        printf("Incorrect arguments!\n"); // 잘몬된 인자 수를 알립니다.
        printf("Usage: %s <inputfile.spl>", argv[0]); // 올바른 사용법을 안내합니다.
        return 1; // 에러 코드 1로 종료합니다.
    }

    filePtr = fopen(argv[1], "r"); // 명령줄 인자로 받은 소스 파일(argv[1])을 읽기모드('r')로 엽니다.
    if (filePtr == NULL) // 파일 열기 실패 여부를 확인합니다.
    {
        printf("Can't open %s. Check the file please", argv[1]); // 파일 열기 실패 메시지를 출력합니다.
        return 2; // 에러 코드 2로 종료합니다.
    }

    while (fgets(line, 4096, filePtr)) // 파일의 끝에 도달할 때까지 파일에서 한 줄씩 읽어 line버퍼에 저장하여 반복합니다.
    {
        int k = 0; // 탭 문자 처리용 루프 변수입니다.

        while (line[k] != '\0') // 읽은 라인의 끝까지 반복합니다.
        {
            if (line[k] == '\t') line[k] = ' '; // 라인에 있는 모든 탭 문자('t')를 공백 문자로 대체합니다.
            k++; // 다음 문자로 이동하기 위해 인덱스를 1 증가시킵니다.
        }

        rstrip(line); // 라인 끝에 있는 개행 문자와 공백을 제거합니다.
        strcpy(lineyedek, line); // 원본 라인을 lineyedek에 복사합니다.

        curLine++; // 현재 라인 번호를 증가시킵니다.
        tempNode.val = -999; // 임시 노드의 값을 초기값(-999)로 설정합니다.
        tempNode.exp_data = ' '; // 임시 노드의 '이름/데이터'를 공백 문자로 초기화합니다.
        tempNode.line = -999; // 임시 노드의 '라인 번호'를 초기값(-999)로 설정합니다.
        tempNode.type = -999; // 임시 노드의 '유형'을 초기값(-999)로 설정합니다.

        if (my_stricmp("begin", line) == 0) // 라인이 begin키워드인지 대소문자 구분 없이 확인합니다.
        {
            if (foundMain) // main 함수 내부에서 begin이 발견되었는지 확인합니다.
            {
                tempNode.type = 4; // 노드 유형을 begin(4)로 설정합니다.
                STACK = Push(tempNode, STACK); // 스택에 푸시하여 새로운 실행 블록의 시작을 기록합니다.
            }
        }
        else if (my_stricmp("end", line) == 0) // 라인이 end키워드인지 대소문자 구분 없이 확인합니다.
        {
            if (foundMain) // main함수 내부에서 end가 발견되었는지 확인합니다.
            {
                int sline; // 함수 호출이 발생한 라인 번호를 저장할 변수입니다.
                tempNode.type = 5; // 노드 유형을 end(5)로 설정합니다.
                STACK = Push(tempNode, STACK); // 스택에 푸시하여 실행 블록의 끝을 기록합니다.

                sline = GetLastFunctionCall(STACK); // 스택에서 가장 최근의 함수 호출 (type == 3)라인 번호를 찾습니다.
                if (sline == 0) // 함수 호출이 없었다면 (현재 코드가 메인 함수의 끝이라면)
                {
                    printf("Output=%d", LastExpReturn); // 최종 결과 값을 출력합니다.
                }
                else // 함수 호출이 있었다면 (현재 코드가 호출된 함수의 끝이라면)
                {
                    int j; // 루프 변수입니다.
                    int foundCall = 0; // 함수 호출 기록을 찾았는지 여부 플래그입니다.
                    LastFunctionReturn = LastExpReturn; // 현재 함수의 최종 결과 값을 반환 값으로 저장합니다.
 
                    fclose(filePtr); // 현재 파일 포인터를 닫고
                    filePtr = fopen(argv[1], "r"); // 파일을 다시 열어 처음부터 다시 읽을 준비를 합니다.
                    curLine = 0; // 현재 라인 번호를 0으로 초기화합니다.
                    for (j = 1; j < sline; j++) // 함수 호출이 발생했던 라인까지 파일을 다시 읽어 파일 포인터를 이동시킵니다.
                    {
                        fgets(dummy, 4096, filePtr); // 더미 버퍼에 라인을 읽어 포인터만 이동시킵니다.
                        curLine++; // 라인 번호를 업데이트합니다.
                    }

                    while (foundCall == 0) // 스택에서 함수 호출 기록 (type == 3)을 찾을 때까지 반복합니다.
                    {
                        Pop(&tempNode, STACK); // 스택에서 노드를 제거합니다.
                        if (tempNode.type == 3) foundCall = 1; // 함수 호출 노드를 찾으면 루프를 종료합니다.
                    }
                }
            }
        }
        else // begin이나 end가 아닌 다른 라인의 경우입니다.
        {
            firstword = strtok(line, " "); // 라인에서 첫 번째 단어를 공백을 기준으로 분리합니다.
            if (!firstword) continue; // 첫 단어가 없다면 다음 라인으로 건너뜁니다.

            if (my_stricmp("int", firstword) == 0) // 첫 단어가 int인지 확인합니다.
            {
                if (foundMain) // main함수 내부에서만 변수를 선언할 수 있습니다.
                {
                    tempNode.type = 1; // 노드 유형을 변수를 나타내는 1로 설정합니다.
                    firstword = strtok(NULL, " "); // 다음 토큰을 가져옵니다. 이것이 변수 이름이 됩니다.
                    if (!firstword) continue; // 변수 이름이 없다면 건너뜁니다.
                    tempNode.exp_data = firstword[0]; // 변수 이름의 첫 글자를 노드 데이터에 저장합니다.

                    firstword = strtok(NULL, " "); // 다음 토큰을 가져옵니다. 이것은 '='기호이거나 초기값 자체가 될 수 있습니다.
                    if (!firstword) continue; // 토큰이 없다면 건너뜁니다.

                    if (my_stricmp("=", firstword) == 0) // 다음 토큰이 대소문자 구분 없이 '='기호인지 확인합니다.
                    {
                        firstword = strtok(NULL, " "); // '='가 있다면 다음 토큰을 가져옵니다. 이것이 초기 값이 됩니다.
                        if (!firstword) continue; // 초기 값이 없다면 건너뜁니다.
                    }

                    tempNode.val = atoi(firstword); // 문자열 형태의 초기 값을 정수형으로 변환하여 노드의 값으로 저장합니다.
                    tempNode.line = 0; // 변수 노드에는 라인 정보가 필요 없으므로 0으로 설정합니다.
                    STACK = Push(tempNode, STACK); // 변수 정보 노드를 메인 스택에 푸시합니다.
                }
            }
            else if (my_stricmp("function", firstword) == 0) // 첫 단어가 function인지 확인합니다.
            {
                firstword = strtok(NULL, " "); // 함수 이름을 가져옵니다.
                if (!firstword) continue; // 함수 이름이 없다면 건너뜁니다.

                tempNode.type = 2; // 노드 유형을 함수 정의 (function, 2)로 설정합니다.
                tempNode.exp_data = firstword[0]; // 함수 이름의 첫 글자를 저장합니다.
                tempNode.line = curLine; // 함수 정의의 시작 라인 번호를 저장합니다.
                tempNode.val = 0; // 값은 사용되지 않습니다.
                STACK = Push(tempNode, STACK); // 함수 정의 정보를 스택에 푸시합니다.

                if (firstword[0] == 'm' && firstword[1] == 'a' && firstword[2] == 'i' && firstword[3] == 'n') // 함수 이름이 main인지 확인합니다.
                {
                    foundMain = 1; // main 함수가 발견되었음을 나타내는 플래그를 설정합니다.
                }
                else // main이 아닌 다른 함수인 경우입니다.
                {
                    if (foundMain) // main함수 실행 중 이 라인에 도달했다면
                    {
                        firstword = strtok(NULL, " "); // 함수의 매개변수 이름을 가져옵니다.
                        if (!firstword) continue; // 매개 변수 이름이 없다면 건너뜁니다.
                        tempNode.type = 1; // 매개변수를 변수(var) 유형인 1로 처리합니다.
                        tempNode.exp_data = firstword[0]; // 매개변수 이름의 첫 글자를 노드 데이터에 저장합니다.
                        tempNode.val = CalingFunctionArgVal; // 함수 호출 시 계산되어 저장된 인자 값을 매개변수의 값으로 설정합니다.
                        tempNode.line = 0; // 변수이므로 라인 정보는 0으로 설정합니다.
                        STACK = Push(tempNode, STACK); // 이 매개변수 노드를 메인 스택에 푸시합니다. 따라서 인자 값에 접근할 수 있게 됩니다.
                    }
                }
            }
            else if (firstword[0] == '(') // 첫 단어가 '('로 시작하는 경우를 확인합니다.
            {
                if (foundMain) // main 함수 내부에서만 표현식을 처리합니다.
                {
                    int i = 0; // lineydek을 순회할 인덱스입니다.
                    int y = 0; // postfix 버퍼에 기록할 인덱스입니다.

                    MathStack->top = NULL; // 연산자 스택을 초기화합니다.

                    while (lineyedek[i] != '\0') // 원본 라인의 끝까지 문자 하나씩 순회합니다.
                    {
                        if (isdigit((unsigned char)lineyedek[i])) // 현재 문자가 숫자인지 확인합니다.
                        {
                            postfix[y] = lineyedek[i]; // 숫자는 후위 표기식에 바로 추가합니다.
                            y++; // 후위 표기식 배열의 다음 위치로 인덱스를 증가시킵니다.
                        }
                        else if (lineyedek[i] == ')') // 문자가 ')'인지 확인합니다.
                        {
                            if (!isStackEmpty(MathStack)) // 연산자 스택(MathStack)이 비어 있지 않다면
                            {
                                postfix[y] = PopOp(MathStack); // 스택에서 연산자를 꺼내 후위표기식에 추가합니다.
                                y++; // 후위 표기식 배열의 다음 위치로 인덱스를 증가시킵니다.
                            }
                        }
                        else if (lineyedek[i] == '+' || lineyedek[i] == '-' || lineyedek[i] == '*' || lineyedek[i] == '/') // 현재 문자가 산술 연산자 중 하나인지 확인합니다.
                        {
                            if (isStackEmpty(MathStack)) // 연산자 스택(MathStack)이 비어 있는지 확인합니다.
                            {
                                MathStack = PushOp(lineyedek[i], MathStack); // 스택이 비어 있다면 현재 연산자를 무조건 스택에 푸시합니다.
                            }
                            else // 스택이 비어 있지 않다면 우선순위를 비교해야 합니다.
                            {
                                if (Priotry(lineyedek[i]) <= Priotry(MathStack->top->op)) // 현재 읽은 연산자의 우선순위가 스택 최상위(top) 연산자의 우선순위보다 낮거나 같은지 비교합니다.
                                {
                                    postfix[y] = PopOp(MathStack); // 스택의 최상위 연산자를 먼저 처리해야 하므로 스택에서 꺼내 후위 표기식에 추가합니다.
                                    y++; // 후위 표기식 배열의 다음 위치로 인덱스를 증가시킵니다.
                                    MathStack = PushOp(lineyedek[i], MathStack); // 스택이 비었는지 확인하는 추가 로직 없이 현재 연산자를 스택에 푸시합니다.
                                }
                                else // 현재 읽은 연산자의 우선순위가 스택 최상위 연산자보다 더 높으면
                                {
                                    MathStack = PushOp(lineyedek[i], MathStack); // 스택의 top 연산자보다 우선순위가 높기 때문에 현재 연산자를 스택에 푸시합니다.
                                }
                            }
                        }
                        else if (isalpha((unsigned char)lineyedek[i]) > 0) // 문자가 알파벳인지 확인합니다.
                        {
                            int codeline = 0; // 함수가 정의된 라인 번호를 받을 변수입니다.
                            int dummyint = 0; // GetVal에 전달되는 더미 포인터 변수입니다.
                            int retVal = GetVal(lineyedek[i], &codeline, STACK); // 스택에서 변수 값 또는 함수 라인 정보를 검색합니다.

                            if ((retVal != -1) && (retVal != -999)) // 찾은 것이 변수 값이라면
                            {
                                postfix[y] = (char)(retVal + 48); // 변수 값을 문자로 변환하여 후위 표기식에 추가합니다.
                                y++; // 후위 표기식 배열의 다음 위치로 인덱스를 증가시킵니다.
                            }
                            else // 찾은 것이 함수이거나 아무것도 아닐 때
                            {
                                if (LastFunctionReturn == -999) // 함수 호출 복귀 중이 아니라면
                                {
                                    int j; // 루프 변수입니다.
                                    tempNode.type = 3; // 노드 유형을 함수 호출 (function call, 3)로 설정합니다.
                                    tempNode.line = curLine; // 함수 호출이 발생한 현재 라인 번호를 저장합니다.
                                    STACK = Push(tempNode, STACK); // 함수 호출 정보를 스택에 푸시합니다.

                                    CalingFunctionArgVal = GetVal(lineyedek[i + 2], &dummyint, STACK); // 호출 인자에서 변수 a의 값을 스택에서 찾아 저장합니다.

                                    fclose(filePtr); // 현재 파일 포인터를 닫고
                                    filePtr = fopen(argv[1], "r"); // 파일을 다시 열어 처음부터 다시 읽을 준비를 합니다.
                                    curLine = 0; // 현재 라인 번호를 0으로 초기화합니다.

                                    for (j = 1; j < codeline; j++) // 호출할 함수가 정의된 라인까지 파일을 읽어 파일 포인터를 이동시킵니다.
                                    {
                                        fgets(dummy, 4096, filePtr); // 더미 버퍼에 라인을 읽어 포인터만 이동시킵니다.
                                        curLine++; // 라인 번호를 업데이트 합니다.
                                    }

                                    WillBreak = 1; // 현재 라인 처리를 중단하고
                                    break; // 바깥 루프를 즉시 종료합니다.
                                }
                                else // 함수 호출 복귀 중이라면,
                                {
                                    postfix[y] = (char)(LastFunctionReturn + 48); // 이전 함수의 반환 값을 후위 표기식에 추가합니다.
                                    y++; // // 후위 표기식 배열의 다음 위치로 인덱스를 증가시킵니다.
                                    i = i + 3; // 함수 호출 구문을 건너뛰기 위해 인덱스를 조정합니다.
                                    LastFunctionReturn = -999; // 함수 반환 값 임시 저장 변수를 초기화합니다.
                                }
                            }
                        }
                        i++; // 다음 문자로 인덱스를 이동시킵니다.
                    }

                    if (WillBreak == 0) // 함수 호출로 인해 중단되지 않았다면
                    {
                        while (!isStackEmpty(MathStack)) // 연산자 스택에 남아 있는 모든 연산자를
                        {
                            postfix[y] = PopOp(MathStack); // 꺼내서 후위 표기식에 추가합니다. 
                            y++; // 후위 표기식 배열의 다음 위치로 인덱스를 증가시킵니다.
                        }

                        postfix[y] = '\0'; // 후위 표기식 문자열의 끝을 표시합니다.

                        i = 0; // 후위 표기식을 순회할 인덱스를 초기화합니다.
                        CalcStack->top = NULL; // 계산 스택을 초기화합니다.
                        while (postfix[i] != '\0') // 후위 표기식의 끝까지 반복합니다.
                        {
                            if (isdigit((unsigned char)postfix[i])) // 문자가 숫자인지 확인합니다.
                            {
                                CalcStack = PushPostfix(postfix[i] - '0', CalcStack); // 문자를 실제 정수 값으로 변환하여 계산 스택에 푸시합니다.
                            }
                            else if (postfix[i] == '+' || postfix[i] == '-' || postfix[i] == '*' || postfix[i] == '/') // 현재 문자가 산술 연산자 중 하나인지 확인합니다.
                            {
                                val1 = PopPostfix(CalcStack); // Postfix 계산에서 먼저 Pop된 값이 연산의 오른쪽 항(val1)이 됩니다.
                                val2 = PopPostfix(CalcStack); // 두 번째 Pop된 값이 연산의 왼쪽 항(val2)이 됩니다. 

                                switch (postfix[i]) // 현재 연산자에 따라 계산을 수행합니다.
                                {
                                case '+': resultVal = val2 + val1; break; // val2 + val1 계산
                                case '-': resultVal = val2 - val1; break; // val2 - val1 계산
                                case '/': resultVal = val2 / val1; break; // val2 / val1 계산
                                case '*': resultVal = val2 * val1; break; // val2 * val1 계산
                                }
                                CalcStack = PushPostfix(resultVal, CalcStack); // 계산 결과를 다시 계산 스택에 푸시합니다. 이 결과는 다음 연산의 피연산자가 됩니다.
                            }
                            i++; // 후위 표기식의 다음 문자로 인덱스를 이동시킵니다.
                        }

                        LastExpReturn = CalcStack->top->val; // 스택의 최상위(top)에 있는 최종 결과 값을 LastExpReturn 변수에 저장합니다.
                    }
                    WillBreak = 0; // 함수 호출로 인해 중단되지 않았으므로 WillBreak 플래그를 다시 0으로 초기화합니다.
                }
            }
        }
    }

    fclose(filePtr); // 파일을 닫습니다.
    STACK = FreeAll(STACK); // 메인 스택의 모든 메모리를 해제합니다.

    printf("\nPress a key to exit..."); // 프로그램 종료 전 사용자에게 키 입력을 기다리도록 안내합니다.
    getchar(); // 키 입력을 기다립니다.
    return 0; // 프로그램 정상 종료를 알립니다.
} // main함수의 마지막 입니다.

static Stack* FreeAll(Stack* stck) // 주 스택의 모든 노드 메모리를 해제하는 함수입니다.
{
    Node* head = stck->top; // head가 포인터를 스택의 top으로 초기화합니다.
    while (head) { // head가 NULL이 될 때까지 반복합니다.
        Node* temp = head; // 해제할 현재 노드를 temp에 임시 저장합니다.
        head = head->next; // head를 다음 노드로 이동시킵니다.
        free(temp); // 이전 노드의 메모리를 해제합니다.
    }
    stck->top = NULL; // 스택의 top을 NULL로 설정합니다.
    return NULL; // NULL을 반환합니다.
} // FreeAll 함수의 마지막입니다.

static int GetLastFunctionCall(Stack* stck) // 스택에서 가장 최근의 함수 호출 라인 번호를 찾아 반환하는 함수입니다.
{
    Node* head = stck->top; // 스택의 top부터 검색을 시작합니다.
    while (head) { // 스택의 끝까지 반복합니다.
        if (head->type == 3) return head->line; // 노드 유형이 함수 호출(3)이면 라인 번호를 즉시 반환합니다.
        head = head->next; // 다음 노드로 이동합니다.
    }
    return 0; // 함수 호출 기록을 찾지 못하면 0을 반환합니다.
} // GetLastFunctionCall 함수의 마지막입니다.

static int GetVal(char exp_name, int* line, Stack* stck) // 주어진 이름(exp_name)에 해당하는 변수 값 또는 함수 라인 정보를 스택에서 검색하여 반환하는 함수입니다.
{
    Node* head; // 스택을 순회할 임시 포인터 변수입니다.
    *line = 0; // 함수 정의 라인 번호를 반환할 포인터 변수를 0으로 초기화합니다.
    if (stck->top == NULL) return -999; // 비어 있다면, 검색 실패를 나타내는 -999를 반환하고 종료합니다.
    head = stck->top; // 스택 순회를 최상위(top) 노드부터 시작합니다.
    while (head) { // 스택의 끝(NULL)에 도달할 때까지 노드를 순회하며 반복합니다.
        if (head->exp_data == exp_name) // 현재 노드에 저장된 이름(exp_data)이 찾고 있는 이름(exp_name)과 일치하는지 확인합니다.
        {
            if (head->type == 1) return head->val; // 변수의 값(val)을 즉시 반환하고 검색을 종료합니다.
            // 노드 유형이 함수 정의 (function)인 경우
            else if (head->type == 2) {
                *line = head->line; // 함수의 정의 라인 번호를 외부 포인터에 저장합니다.
                return -1; // 함수임을 알리는 특수 값(-1)을 반환하고 검색을 종료합니다.
            } 
        }
        head = head->next; // 현재 노드가 찾는 항목이 아니므로 다음 노드로 이동하여 검색을 계속합니다.
    }
    // 스택의 끝까지 순회했지만 일치하는 항목을 찾지 못한 경우
    return -999; // 검색 실패를 나타내는 -999를 반환합니다.
}

static int my_stricmp(const char* a, const char* b) // 대소문자를 구분하지 않고 문자열을 비교하는 함수입니다.
{
    unsigned char ca, cb; // 비교할 두 문자를 소문자로 변환하여 저장할 변수를 선언합니다.

    // 두 문자열 중 하나라도 널 문자에 도달할 때까지 반복합니다.
    while (*a || *b) {
        ca = (unsigned char)tolower((unsigned char)*a); // 첫 번째 문자열의 현재 문자를 소문자로 변환합니다.
        cb = (unsigned char)tolower((unsigned char)*b); // 두 번째 문자열의 현재 문자를 소문자로 변환합니다.

        // 소문자로 변환된 두 문자가 서로 다르면
        if (ca != cb) return (int)ca - (int)cb; // 두 문자의 ASCII 값 차이를 반환하고 즉시 종료합니다.
        if (*a) a++; // 두 문자가 같고 첫 번째 문자열이 끝이 아니라면 다음 문자로 포인터를 이동합니다.
        if (*b) b++; // 두 문자가 같고 두 번째 문자열이 끝이 아니라면 다음 문자로 포인터를 이동합니다.
    }
    return 0; // 루프가 종료될 때까지 두 문자열이 모두 일치했다면 0을 반환합니다.
}

static void rstrip(char* s) // 문자열의 현재 길이를 얻습니다.
{
    size_t n = strlen(s); // 문자열의 현재 길이를 'n'에 저장합니다.

    // 조건
    // 1. n > 0 : 문자열이 비어 있지 않아야 합니다.
    // 2. 끝 문자(s[n-1])가 '\n', '\r', ' ' 중 하나여야 합니다. 
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' '))

        // 반복문의 조건이 참인 동안:
        // 1. --n : 길이 'n'을 1 감소시킵니다. (s[--n]을 사용하면 널 문자를 삽입할 위치가 됩니다.)
        // 2. s[--n] = '\0' : 이전 끝 문자 위치에 '\0'를 삽입하여 문자열의 끝을 재정의합니다.
        // 이 과정을 통해 문자열의 실제 길이가 줄어들고 후행 공백이 제거됩니다.
        s[--n] = '\0';
}