#ifndef ARRAY_H
#define ARRAY_H

int* addElement(int* array, int* size, int element);
int* removeElement(int* array, int* size, int element);

// EXAMPLE
// 
// int main() {
//     int size = 0;
//     int* array = NULL;
// 
//     array = addElement(array, &size, 1);
//     array = addElement(array, &size, 2);
//     array = addElement(array, &size, 3);
//     array = addElement(array, &size, 1);     // will print message and not add any element
// 
//     array = removeElement(array, &size, 2);
//     array = removeElement(array, &size, 1);
//     array = removeElement(array, &size, 3);
//     array = removeElement(array, &size, 3);  // will do nothing
// 
//     array = addElement(array, &size, 3);
// 
//     for (int i = 0; i < size; i++) {
//         printf("%d ", array[i]);             // result will be: 3   
//     }
//     printf("\n");
// 
//     free(array);
// }

#endif