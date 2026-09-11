#include <stdio.h>

// Write your sumArray function here
int sumArray(int arr[], int n){
    int sum = 0;

    for(int i = 0 ; i<n; i++){
        sum += arr[i];
    }

    return sum;
}

int main() {
    int n;
    scanf("%d", &n);
    
    int arr[n];
    for(int i = 0; i < n; i++) {
        scanf("%d", &arr[i]);
    }
    
    // Call the sumArray function and print the result
    int sum = sumArray(arr, n);
    printf("Sum: %d\n", sum);
    
    return 0;
}