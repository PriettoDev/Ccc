#include <stdio.h>

void getElement(int arr[], int size, int index) {
    // Write your code here
    if(index<size){
        printf("%d", arr[index]);
    }
    else{
        printf("Index out of bounds");
    }
}

int main() {
    int size, index;
    
    // Read the size of the array
    scanf("%d", &size);
    
    int arr[size];
    
    // Read array elements
    for(int i = 0; i < size; i++) {
        scanf("%d", &arr[i]);
    }
    
    // Read the index to access
    scanf("%d", &index);
    
    getElement(arr, size, index);
    
    return 0;
}