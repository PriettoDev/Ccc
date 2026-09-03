#include <stdio.h>

void printDiagonal(int matrix[][100], int size) {
    for (int i=0; i<size; i++){
        for(int j = 0; j < size; j++){
            if(i==j){
                printf("%d ", matrix[i][j]);
            }
        }
    }
}

int main() {
    int size;
    scanf("%d", &size);
    
    int matrix[100][100];
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            scanf("%d", &matrix[i][j]);
        }
    }
    
    printDiagonal(matrix, size);
    
    return 0;
}