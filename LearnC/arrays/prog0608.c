#include <stdio.h>

#define DIM 3
#define BRANCO ' '

void ini(char tab[][DIM]);
void print(char tab[DIM][DIM]);

void ini(char tab[][DIM]){
    for(int i=0; i<DIM; i++){
        for(int j; j<DIM; j++){
            tab[i][j]=BRANCO;
        }
    }
}

void print(char tab[DIM][DIM]){
    for(int i=0; i<DIM; i++){
        for(int j=0; j<DIM; j++){
            printf("%c %c", tab[i][j], j==DIM-1 ? ' ' : '|');

            if(i!=DIM-1){
                printf("\n---------------");
            }
            putchar('\n');
        }
    }
}

int main(void){
    char tabuleiro[DIM][DIM];
    int posX, posY;
    char ch = 'X';
    int nJogadas = 0;

    ini(tabuleiro);
    while(1){
        print(tabuleiro);

        printf("\nIntroduza a quantidade de linhas e colunas: ");
        scanf("%d %d", &posX, &posY);
        posX--;
        posY--;

        if(tabuleiro[posX][posY] == BRANCO){
            tabuleiro[posX][posY] = ch = (ch=='0') ? 'X' : '0';
            nJogadas++;
        }
        else{
            printf("Posicao ja ocupada\nJogue novamente\n");
        }
        if(nJogadas==DIM*DIM){
            break;
        }
    }
    print(tabuleiro);

    return 0;
}