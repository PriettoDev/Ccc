#include <stdio.h>

int main(void){
    int i;
    float total, sal[12];

    for(i=0; i<12; i++){
        printf("Insira o salario do mes %d: ", i+1);
        scanf("%f", &sal[i]);
    }

    puts(" Mes      Valor");

    for(i=0, total=0.0; i<12; i++){
        printf(" %d        %7.2f\n", i+1, sal[i]);
        total += sal[i];
    }

    printf("Valor total final: %.2f", total);
    return 0;
}