#include <stdio.h>

int calculateArea(int a, int b);

int main() {
    int a, b;
    scanf("%d", &a);
    scanf("%d", &b);

    int result = calculateArea(a, b);

    printf("The area is equal to: %d", result);

    return 0;
}

int calculateArea(int a, int b){
    return a*b;
}