/*
 * Challenge 02 — Stack Buffer Overflow (심화: 삼각 인덱싱 off-by-one)
 *
 * [시나리오]
 *   파스칼의 삼각형을 스택 위 "1차원" 배열에 삼각 인덱싱으로 채운다.
 *   파스칼의 삼각형(Pascal's Triangle)은 숫자를 삼각형 모양으로 배치한 것으로, 다음과 같은 규칙을 가진다.
 *    - 맨 위는 항상 1
 *    - 양쪽 가장자리는 항상 1
 *    - 가운데 숫자는 바로 위의 두 숫자의 합
 *              1
 *           1   1
 *         1   2   1
 *       1   3   3   1
 *     1   4   6   4   1
 *   1   5  10  10   5   1
 *
 *  파스칼의 삼각형을 2차원 배열로 정리하면 하기와 같다.
 *   1
 *   1 1
 *   1 2 1
 *   1 3 3 1
 *
 *   1차원 배열로 정리할때는, 행 i 의 원소 j 는 인덱스  idx = i*(i+1)/2 + j  에 저장한다.
 *   배열 크기는 정확히 ROWS 개 행(0..ROWS-1)을 담도록 SIZE = ROWS*(ROWS+1)/2 로 잡았다.
 *
 * [기대 동작]
 *   삼각형을 만들고 각 행의 합(=2^i)을 출력한 뒤 정상 종료.
 *
 * [증상]
 *   행 루프가 `i <= ROWS` 로 도는 바람에 행이 하나 더 생성된다.
 *   그 행의 인덱스는 idx = SIZE + j 가 되어 스택 배열의 끝을 넘어 쓴다.
 *   스택 카나리(스매싱 보호)가 훼손되어 main 반환 시 "stack smashing detected"
 *   로 SIGABRT. 삼각 인덱싱 산술에 가려 off-by-one 이 눈에 잘 안 띈다.
 *
 */
#include <stdio.h>
#include <stdlib.h>

#define ROWS 14
enum { SIZE = ROWS * (ROWS + 1) / 2 };   /* 0..ROWS-1 행을 담는 정확한 크기 */
// 14*15/2 = 105

/* 행 i, 열 j 의 삼각 인덱스 */
static int tri_index(int i, int j) {
    return i * (i + 1) / 2 + j;
}

/* 파스칼의 삼각형을 tri[] 에 채운다. */
static void build_pascal(int *tri, int rows) {
    for (int i = 0; i <= rows; i++) {
        for (int j = 0; j <= i; j++) {
            int idx = tri_index(i, j);
            if (j == 0 || j == i) {
                tri[idx] = 1; /* 양 끝은 1 */
            } else {
                int up_left  = tri_index(i - 1, j - 1);
                int up_right = tri_index(i - 1, j);
                tri[idx] = tri[up_left] + tri[up_right];
            }
        }
    }
}

static long row_sum(const int *tri, int i) {
    long sum = 0;
    for (int j = 0; j <= i; j++) sum += tri[tri_index(i, j)];
    return sum;
}

static void print_row(const int *tri, int i) {
    printf("row %2d:", i);
    for (int j = 0; j <= i; j++) printf(" %d", tri[tri_index(i, j)]);
    printf("   (sum=%ld)\n", row_sum(tri, i));
}

int main(void) {
    int tri[SIZE];    

    build_pascal(tri, ROWS);          

    for (int i = 0; i < ROWS; i++) print_row(tri, i);

    printf("SIZE = %d\n", SIZE);

    return 0;
}
