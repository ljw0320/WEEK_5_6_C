/*
 * Challenge 07 — Stack Use After Return (심화: 지역 배열 주소가 탈출)
 *
 * [시나리오]
 *   문자열을 줄 단위로 쪼개, 각 줄의 시작 주소들을 담은 "뷰(LineView)"를 만든다.
 *   split_lines() 가 만든 뷰를 호출자가 받아서 출력한다.
 *
 * [기대 동작]
 *   "alpha / beta / gamma" 세 줄로 쪼갠 뒤, 줄 수와 각 줄 첫 글자의 합을 출력
 *   (lines = 3, checksum = 298).
 * 
 * [공부 포인트]
 * 1. 포인터를 사용할 때는 주소보다 객체의 생명 주기를 먼저 확인한다.
 * 2. 지역변수의 주소를 함수 밖에서 계속 사용하면 안 된다.
 * 3. 필요한 데이터의 수명에 맞게 스택·힙·static 중 적절한 저장 방식을 선택한다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES 8

// 16B = 8 + 4 + 4
typedef struct
{
    char **lines; /* 줄 포인터들의 '배열'을 가리킨다 */
    int count;
} LineView;

// static void view_set(LineView *out, char **arr, int n)
// {
//     out->lines = arr;
//     out->count = n;
// }

// 행 분할후 저장
static void split_lines(LineView *out, char *text)
{    
    int n = 0;

    // 방법 1 _ 힙 메모리 할당
    /*
    char **parts;
    parts = malloc(sizeof(char *) * MAX_LINES);

    for (char *ln = strtok(text, "\n"); ln && n < MAX_LINES; ln = strtok(NULL, "\n"))
    {
        parts[n++] = ln;        
    }

    view_set(out, parts, n);    
    */
    // 방법 2 _ 값 직접 저장  -> 지역변수 사용 x           

    for (char *ln = strtok(text, "\n"); ln && n < MAX_LINES; ln = strtok(NULL, "\n"))    
        out->lines[n++] = ln;        
    
    out->count = n;
    
    /* TODO 상기 코드를 수정하여 결과를 호출자가 준 out 에 직접 채운다(값 반환 아님, 지역 주소 반환 아님). */       
}

// __asm__ : C 코드 안에 어셈블리 코드를 직접 작성할 때 사용하는 GCC 확장 문법
/*
__asm__ volatile(
    "어셈블리 코드"
    : 출력 operand
    : 입력 operand
    : clobber
);
*/
static void warm_stack(void)
{
    char *scratch[MAX_LINES];
    for (int i = 0; i < MAX_LINES; i++)
        scratch[i] = (char *)0x4141414141414141ULL; /* 매핑되지 않은 주소 */
    __asm__ volatile("" ::"r"(scratch) : "memory"); /* 최적화 제거 방지 */
}

int main(void)
{
    char text[] = "alpha\nbeta\ngamma";
    char *parts[MAX_LINES];
    LineView v;
    v.lines = parts;
    split_lines(&v, text);
    warm_stack();

    long checksum = 0;
    for (int i = 0; i < v.count; i++)
        checksum += (unsigned char)v.lines[i][0];

    printf("lines = %d, checksum = %ld\n", v.count, checksum);

    //free(v.lines);
    return 0;
}
