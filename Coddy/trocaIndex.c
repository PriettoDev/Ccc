#include <stdio.h>

void swapElements(int arr[], int size, int index1, int index2) {
    // Swap values using a temporary variable
    int temp = arr[index1];
    arr[index1] = arr[index2];
    arr[index2] = temp;

    // Print array formatted with space separation
    for (int i = 0; i < size; i++) {
        printf("%d%s", arr[i], (i == size - 1) ? "" : " ");
    }
    printf("\n");
}

int main() {
    int size;
    scanf("%d", &size);
    
    int arr[size];
    for (int i = 0; i < size; i++) {
        scanf("%d", &arr[i]);
    }
    
    int index1, index2;
    scanf("%d %d", &index1, &index2);
    
    swapElements(arr, size, index1, index2);
    
    return 0;
}