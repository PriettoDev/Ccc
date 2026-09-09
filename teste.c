#include <stdio.h>

//int main(){
//    printf("Bem-vindos ao /mundo\\ da programaçao \"C\"");
// //    puts("Hello World"); 
//     int num = 123;
//     printf("O valor de num= %d e o valor seguinte = %d\n",num,num+1);
//     printf("O tamanho em bytes de um inteiro eh: %d", sizeof(double));
// ----------------------------------
// char ch1, ch2;
// printf("Introduza um caractere: ");
// scanf("%c", &ch1);
// printf("Introduza outro caractere: ");
// scanf(" %c", &ch2);
// printf("Os caracteres inseridos foram '%c', e '%c'\n", ch1,ch2);
// ----------------------------
// int num;
// printf("Escreva um numero: ");
// scanf("%d", &num);
// printf("O numero digitado eh: %d, cujo caractere correspondente eh o: %c\n", num, (char) num);
// printf("O proximo numero seria o: %d, cujo caractere correspondente eh o: %c\n", num+1, (char) num+1);
// ------------------------
// int dd, mm, aa;
// printf("Digite uma data: ");
// scanf("%d%d%d", &dd, &mm, &aa);
// printf("%d/%d/%d", dd, mm, aa);
// ---------------------
// float salario;
// printf("Digite o seu salario: ");
// scanf("%f", &salario);
// if(salario<=0){
//     printf("Erro!");
// }
// else if(salario<1000 && salario>0){
//     salario = salario-(salario*0.05);
//     printf("Seu salario com os impostos eh: %.2f", salario);
// }
// else{
//     salario = salario-(salario*0.15);
//     printf("Seu salario com os impostos eh: %.2f", salario);
// }

// int factorial(int n);

// int main(){
//     int n;
//     printf("Digite um numero inteiro: ");
//     scanf("%d", &n);
//     printf("O fatorial de %d e: %d", n, factorial(n));
//     return 0;
// }

// int factorial(int n){
//     int fat=1;
//     for (int i=n; i>=1; i--)
//         fat=fat*i;
//     return fat;
// }

// int fibonacci(int n);

// int main(){
//     int n;
//     printf("Digite o termo desejado: ");
//     scanf("%d", &n);
//     printf("O elemento presente no termo %d eh: %d", n, fibonacci(n));
//     return 0;
// }

// int fibonacci(int n){
//         int a=1, b=0, fib;
//     for (int i=1; i<=n; i++){
//         fib=a+b;
//         a=b;
//         b=fib;
//     }
//     return fib;
// }

// long fibonacci(long n);

// int main(){
//     long n;
//     printf("Digite o termo desejado: ");
//     scanf("%ld", &n);
//     printf("O elemento presente no termo %ld eh: %ld", n, fibonacci(n));
//     return 0;
// }

// long fibonacci(long n){
//     if ((n==1) || (n==2)) //solução trivial
//         return 1;
//     else //solução geral
//         return fibonacci(n-1)+fibonacci(n-2);
// }

