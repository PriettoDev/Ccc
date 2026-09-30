#include <stdio.h>

int my_isdigit(char ch){
    return (ch >=  '0' && ch<='9');
}

//está função irá printar todos os caracteres, do teclado, que não forem digitos entre 0 e 9.

int main(void){
    char c;
    while(1){
        c = getchar();
        if(!my_isdigit(c)){ //caso não seja um digito, entre 0 e 9, irá fazer o comando dentro do if.
            putchar(c);
        }
        return 0;
    }
}