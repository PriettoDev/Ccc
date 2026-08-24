#include <stdio.h>

int calculate(int a, int b, char op);

int main() {
    int a, b;
    char op;
    scanf("%d", &a);
    scanf("%d", &b);
    scanf(" %c", &op);

    if(b == 0){
        printf("Invalid input");
    }
    else{
        int result = calculate(a, b, op);
        printf("%d", result);
    }
    
    return 0;
}

int calculate(int a, int b, char op){
    if (op == '+'){
        return a+b;
    }
    else if(op == '-'){
        return a-b;
    }
    else if(op == '/'){
        return a/b;
    }
    else{
        return a*b;   
    }
}