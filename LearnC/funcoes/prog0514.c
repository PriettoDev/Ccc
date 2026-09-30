#include <stdio.h>

double my_power(double, int); //prototipo de função

int main(void){
    double base;
    int exp;

    base = 2.0; exp = 4;
    printf("%.3f^%d = %.4f\n", base, exp, my_power(base, exp));

    base = 1.234; exp = 7;
    printf("%.3f^%d = %.4f\n", base, exp, my_power(base, exp));

    base = 10.0; exp = 0;
    printf("%.3f^%d = %.4f\n", base, exp, my_power(base, exp));

    return 0;
}

double my_power(double x, int n){
    float res = 1.0;
    while(n-- > 0){
        res *= x;
    }
    return res;
}