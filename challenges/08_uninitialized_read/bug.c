/*
 * Challenge 08 — 미초기화 포인터 읽기 (심화: 더티 힙 재사용)
 *
 * [시나리오]
 *   희소 행렬(sparse matrix)을 "행 포인터 표"로 표현한다.
 *   희소 행렬(Sparse Matrix)은 대부분의 원소가 0인 행렬을 말한다.
 * 예시 :
 * 0 0 0 0 5
 * 0 0 3 0 0
 * 0 0 0 0 0
 * 7 0 0 0 0
 * 0 0 0 9 0
 *
 * rows[i] 는 i 번째 행
 * 배열을 가리키며, 실제 데이터가 있는 행만 malloc 해서 연결한다.
 *
 * [기대 동작]
 *   채운 행만 안전하게 합산해 출력.
 *
 * [환경 의존성 — 반드시 제공된 Docker(리눅스+glibc) 안에서 실행]
 *   이 실습이 "방금 free 한 청크를 곧바로 재사용해 크래시한다"고 장담할 수 있는 이유는
 *   실행 환경을 리눅스 + glibc(ptmalloc2)로 고정했기 때문이다.
 *   - glibc 2.26+ 의 tcache 는 작은 블록을 free 하면 크기별 통(bin)에 LIFO(스택)로
 *     넣어두고, "같은 크기"를 다시 malloc 하면 방금 넣은 블록을 그대로 되돌려준다.
 *     → dirty_heap() 가 0xAB 로 더럽혀 free 한 256B 청크를, 직후 make_matrix() 의
 *       malloc(256B) 이 거의 결정적으로 다시 받는다(그 사이 같은 크기 할당이 없으므로).
 *   - 반면 C 표준이 보장하는 것은 "malloc 값은 불특정(쓰레기)"뿐이다. tcache/fastbin
 *     같은 재사용 세부는 glibc 전용 구현이며, macOS(libmalloc)·Windows(HeapAlloc)·
 *     musl 등 다른 할당기에서는 동작이 달라 크래시가 다르게 나거나 우연히 안 날 수 있다.
 *   → 그래서 결과의 일관성을 위해 이 코드는 반드시 제공된 리눅스/glibc Docker 에서
 *     실행한다. (교훈 자체 "미초기화 = NULL 아닌 쓰레기" 는 OS 무관하게 항상 참)
 * 
 * [교훈]
 * 1. malloc()은 초기화하지 않는다.
 * 특히 포인터 배열을 malloc()으로 만들었다고 해서 각 포인터가 자동으로 NULL이 되는 게 아니다.
 * 
 * 2. "할당된 포인터"와 "존재하지 않는 포인터"를 구분할 값이 필요하다. 
 * 여기서는 NULL이 그 역할을 한다.
 * 
 * 3. 자료구조의 생성·사용·해제 규칙은 서로 일치해야 한다. 
 * 현재 코드는 모든 행을 할당하면서 짝수 행만 free()하기 때문에 생성 규칙과 해제 규칙이 맞지 않는다.
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROWS 32
#define COLS 4

/* 힙을 '더럽혀' 두어, 이후 같은 크기 할당이 쓰레기 값을 물려받게 만든다.
   (실무에서 흔한 '이전에 쓰고 free 한 청크의 잔여물' 상황을 재현) */
static void dirty_heap(void)
{
    void *scratch = malloc(ROWS * sizeof(int *));
    if (scratch)
    {
        memset(scratch, 0xAB, ROWS * sizeof(int *));
        free(scratch); /* glibc tcache 로 반환 → 같은 크기 malloc 이 이 블록을
                          LIFO 로 되돌려받는다(리눅스+glibc 고정이라 결정적). */
    }
}

// 행렬 생성

static int **make_matrix(void)
{
    //int **rows = malloc(ROWS * sizeof(int *)); // int * 타입을 가리킴(각 원소 크기 8B인 길이 32짜리 배열)
    int **rows = calloc(ROWS, sizeof(int *)); // _ljw add . NULL 초기화
    
    if (!rows)
    {
        perror("malloc");
        exit(1);
    }

    for (int i = 0; i < ROWS; i += 2)   // 1, 3, 5, ... 등 홀수 행 초기화 안됨
    {
        int *r = malloc(COLS * sizeof(int)); // int 타입을 가리킴(각 원소 크기 4B인 길이 4짜리 배열)

        if (!r)
        {
            perror("malloc");
            exit(1);
        }

        for (int j = 0; j < COLS; j++)       // 각 배열의 원소 값 지정
            r[j] = i * COLS + j;

        rows[i] = r;
    }
    return rows;
}

static long row_sum(int **rows, int nrows)
{
    long total = 0;
    for (int i = 0; i < nrows; i++)
    {
        if (rows[i] == NULL) 
            continue;

        for (int j = 0; j < COLS; j++)
        {
            total += rows[i][j]; // crash
        }
    }
    return total;
}

int main(void)
{
    dirty_heap();

    int **rows = make_matrix();
    printf("summing %dx%d matrix...\n", ROWS, COLS);

    long s = row_sum(rows, ROWS);

    printf("sum = %ld\n", s);

    for (int i = 0; i < ROWS; i += 2)
        free(rows[i]);

    free(rows);
    return 0;
}
