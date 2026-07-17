#include <stdio.h>

float fahrenheitToCelsius(float C, float F){
    C = (F - 32) * 5/9;
    return C;
}
int main() {
    
    float F, C;
    scanf("%f", &F);
    
    float temperature = fahrenheitToCelsius(C, F);
    printf("%.1f °C", temperature);

    return 0;
}