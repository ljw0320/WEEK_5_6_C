/*
 * Challenge 03 — Heap Buffer Overflow (심화: 동적 배열 성장 버그)
 *
 * [시나리오]
 *   자동 성장하는 정수 동적 배열 IntList (init/ensure/push/sum). 용량이 부족하면
 *   list_ensure() 가 용량을 2배로 늘리고 realloc 한다. 이 리스트로 큰 수열을
 *   만들어 합을 구한다.
 *
 * [기대 동작]
 *   0..N-1 을 100 으로 나눈 나머지를 리스트에 넣고, 길이·용량·합을 출력한 뒤 정상 종료.
 *
 *
 */
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int   *data;
    /* [Thinking Point]
     * 개수/크기를 담는 len, cap 을 왜 int 가 아니라 size_t 로 선언할까?
     *   tip 1. size_t 는 "이 플랫폼에서 표현 가능한 가장 큰 객체 크기"를 담도록 만든
     *          부호 없는(unsigned) 정수 타입이다. malloc/sizeof/strlen 의 타입도 size_t 다.
     *   tip 2. int 는 보통 32비트라 약 21억(2^31-1)에서 넘치고, 음수도 가능하다.
     *          원소가 그보다 많아지거나 cap*sizeof(int) 계산이 커지면 int 는 오버플로된다.
     *   생각해보기: 크기를 int 로 두면 어떤 버그가 생길 수 있을까?
     *   1_개수와 크기는 0이상의 정수여야 하는데, 만약 21억보다 큰 값이 들어가면 음수가 나옴. 표현할 수 있는 값을 초과했기 때문.
     *     size_t는 0~1844경 까지 담을 수 있기 때문에 더 유리.
     * 
     *   2_ C의 메모리 크기나 개수와 관련된 표준 기능들은 usigned long를 사용. sizeof, malloc등
     * 
     *   3_ 연산 중에 문제가 생길 수 있음. 예를들어 1000만 이라는 숫자를 사용했지만 여기에 1000 정도의 숫자만 곱해도 범위를 초과한다.
     *      사이즈를 2배씩 10번만 늘려도 문제가 생긴다는 것.
     */
    size_t len;
    size_t cap;
} IntList;

// list 초기화 : cap, len, data 메모리 동적 할당
static void list_init(IntList *l) {
    l->cap  = 8;
    l->len  = 0;
    l->data = malloc(l->cap * sizeof(int)); // => 최초 32 바이트
    if (!l->data) { perror("malloc"); exit(1); }
}

// data 메모리 사이즈 확장
static void list_ensure(IntList *l, size_t need) {
    if (need <= l->cap) 
        return;

    size_t newcap = l->cap ? l->cap * 2 : 8; // l->cap이 0이면 8, 아니면 l->cap*2 

    while (newcap < need) // 여전히 요구하는 값보다 작다면 2배씩 크기 증가
        newcap *= 2; 

    // l->data 메모리 크기 재할당 ※realloc이후 주소가 바뀔 수도 있으므로 임시포인터 사용
    // int *p = realloc(l->data, l->cap * sizeof(int)); // 여기서 죽음! _ljw comment out
    int *p = realloc(l->data, newcap * sizeof(int));    // _ljw add : l->cap의 크기는 아직 확장되지 않았는데 l->cap만큼 재할당해서 문제가됨. newcap으로 변경
    
    if (!p) { perror("realloc"); free(l->data); exit(1); } // NULL이면 에러 코드 표시 후 종료

    l->data = p;
    l->cap  = newcap;
}

// 1. len이 cap과 같으면 list_ensure 실행하여 공간 확장
// 2. 데이터 배열 len+1 인덱스에 x값 대입 
static void list_push(IntList *l, int x) {
    if (l->len == l->cap) 
        list_ensure(l, l->cap + 1);

    l->data[l->len++] = x;
}


static long long list_sum(const IntList *l) {
    long long s = 0;
    for (size_t i = 0; i < l->len; i++) 
        s += l->data[i];
    return s;
}

static void list_free(IntList *l) {
    free(l->data);
    l->data = NULL;
    l->len = l->cap = 0;
}

int main(void) {
    IntList l;
    list_init(&l);

    const int N = 2000000; // 200만번
    for (int i = 0; i < N; i++) { 
        list_push(&l, i % 100); // 100의 나머지 => 0~99        
        //printf("i == %d\n",i); // _ljw comment out
        printf("%d : data[i] == %d\n", i, (&l)->data[i]); // _ljw add
    }

    printf("len=%zu cap=%zu sum=%lld\n", l.len, l.cap, list_sum(&l));
    list_free(&l);
    return 0;
}
