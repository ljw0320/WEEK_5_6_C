/*
 * Challenge 06 — NULL Pointer Dereference (심화: HTTP 헤더 파서)
 *
 * [시나리오]
 *   "Key: Value" 형식의 헤더 블록을 줄 단위로 파싱한다. 각 줄에서 ':' 를 찾아
 *   그 자리를 '\0' 로 끊어 key/value 로 나눈 뒤 목록에 저장한다.
 *
 * [기대 동작]
 *   모든 헤더를 key/value 로 나눠 저장하고 개수와 내용을 출력.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HEADERS 32

// 520 B = 8*32 + 8*32 + 8
typedef struct
{
    char *keys[MAX_HEADERS];
    char *vals[MAX_HEADERS];
    int count;
} Headers;

static char *skip_ws(char *s)
{
    while (*s == ' ' || *s == '\t')
        s++;
    return s;
}

// strtok (text, "\n") : 문자열(char * 타입)을 특정 구분자("\n")를 기준으로 잘라서 하나씩 꺼내는 함수
// \n을 찾은 뒤 \0으로 변경
// 문자열의 첫번째 문자 위치 반환.
// 두번째 호출 부터는 strtok(NULL, "\n")과 같이 문자열 자리에 NULL 삽입
// strtok()가 이전에 어디까지 검사했는지를 내부적으로 기억하고 있기 때문
// \0뒤의 첫번째 문자 위치 반환
// 구분자는 여러개 지정 가능 ex) ", " => , 또는 공백(' ') 둘 중 하나 사용

// 헤더 파싱 함수
// 입력된 text를 key/value 로 나눠 저장
static void parse_headers(char *text, Headers *h)
{
    for (char *line = strtok(text, "\n"); line != NULL; line = strtok(NULL, "\n"))
    {
        // 첫번째 ':' 위치 찾기
        // :가 없으면?
        char *colon = strchr(line, ':');
        
        // ':'를 '\0'로 변환
        // 토큰 key에 저장
        // 공백이나 tab 다음 값 val에 저장
        char *key = line;
        char *val = NULL;        

        if (colon == NULL) 
            continue;

        *colon = '\0'; 
        val = skip_ws(colon + 1);        

        // 헤더 초기화
        if (h->count < MAX_HEADERS)
        {
            h->keys[h->count] = key;
            h->vals[h->count] = val;
            h->count++;
        }
    }
}

int main(void)
{

    char raw[] =
        "Host: example.com\n"   // line : 'H' 포인터
        "Accept: */*\n"         // line : 'A' 포인터  
        "Connection\n"         // line : 'C' 포인터  
        "User-Agent: memdbg-cli\n";          // line : 'U' 포인터  

    Headers h = {.count = 0};
    parse_headers(raw, &h);

    printf("parsed %d headers\n", h.count);
    for (int i = 0; i < h.count; i++)
        printf("  %s = %s\n", h.keys[i], h.vals[i]);
    return 0;
}
