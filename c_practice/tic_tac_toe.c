#include <stdio.h>

int *func(void) 
{ 
    int a = 10; 
    int *p = &a;
    return p; 
}
  
int main(void) 
{ 
    int *q = func();    
    printf("%d", *q);
}

