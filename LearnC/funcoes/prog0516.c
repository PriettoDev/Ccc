#include <stdio.h>

long int n_segundos(int);
long int n_minutos(int);

int main(void){
    int n_hora;
    char tipo;

    printf("Quantas horas?\n");
    scanf("%d", &n_hora);
    printf("Qual a transformação?\ns (segundos)\nm (minutos)\nh (horas)\n");
    scanf(" %c", &tipo);

    switch (tipo)
    {
    case 'S':
    case 's':
        printf("O equivalente em segundo eh: %ld", n_segundos(n_hora));  
        break;
    
    case 'M':
    case 'm':
        printf("O equivalente em minutos eh: %ld", n_minutos(n_hora));  
        break;

    case 'H':
    case 'h':
        printf("O equivalente em horas eh: %d", n_hora);  
        break;

    default:
        printf("Valor inserido invalido!");
        break;
    }
}

long int n_segundos(int n_hora){
    int min = n_hora*60;
    int seg = min*60;

    return seg;
} 

long int n_minutos(int n_hora){
    int min = n_hora*60;

    return min;
} 