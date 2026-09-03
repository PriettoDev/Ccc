#include <stdio.h>

void updateScore(int scores[], int playerNumber, int newScore) {
    // Write your code here
    int index = playerNumber-1;
    scores[index] = newScore;
    printf("Scores: [");
    for(int i = 0; i<5; i++){
        if(i<4){
            printf("%d, ", scores[i]);
        }
        else{
            printf("%d", scores[i]);
        }
    }
    printf("]");
}

int main() {
    int scores[5];
    
    // Read the current scores
    for (int i = 0; i < 5; i++) {
        scanf("%d", &scores[i]);
    }
    
    // Read the player number and new score
    int playerNumber, newScore;
    scanf("%d %d", &playerNumber, &newScore);
    
    // Call the updateScore function
    updateScore(scores, playerNumber, newScore);
    
    return 0;
}