#include <stdio.h>
#define X 1
#define Y 2

char board[3][3];
char initBoard[] = "| - | - | - |\n| - | - | - |\n| - | - | - |\n"; // -에 X또는 O입력

int main()
{
    printf(initBoard);
    return 0;
}

