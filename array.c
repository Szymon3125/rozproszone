#include "array.h"

#include <stdio.h>
#include <stdlib.h>

int* addElement(int* array, int* size, int element) {
    if (array == NULL) {
        array = malloc(sizeof(int));
        array[0] = element;
        (*size) = 1;
        return array;
    }

    for (int i = 0; i < (*size); i++) {
        if (array[i] == element) {
            printf("array: duplicate ellement detected. adding ingored!\n");
            return array;
        }
    }

    (*size)++;
    array = realloc(array, (*size) * sizeof(int));
    array[(*size) - 1] = element;
    return array;
}

int* removeElement(int* array, int* size, int element) {
    if (array == NULL) {
        return NULL;
    }

    int newSize = 0;

    for (int i = 0; i < (*size); i++) {
        if (array[i] != element) {
            array[newSize] = array[i];
            newSize++;
        }
    }

    if (newSize == 0) {
        free(array);
        (*size) = 0;
        return NULL;
    }

    array = realloc(array, newSize * sizeof(int));
    (*size) = newSize;
    return array;
}

