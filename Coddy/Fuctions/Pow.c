#include <stdio.h>
#include <math.h>

int power(int x, int n); //prototipo de função

int main() { //main
    int x, n;
    scanf("%d %d", &x, &n);
    
    printf("%d", power(x, n));
    return 0;
}

int power(int x, int n){ //implementação da função
    if(n==0){
        return 1;
    }
    else{
        double result = pow(x, n);
        return result;
    }
}