//
// =========
// 1lang1server/c-server/DataStructures
//
// =========
// 
//
// Created by Samandar Komil on 18/04/2025.
// 
//
// 
// dynamic_array.c


#include <stdlib.h>
#include "dynamic_array.h"


// Utility functions
int resize_array(DynamicArray* self, int new_capacity){
    int *temp_arr = realloc(self->array, new_capacity * sizeof(int));
    if (!temp_arr) return DA_ERROR_ALLOC;
    self->array = temp_arr;
    self->capacity = new_capacity;
    return 0;
}


// DynamicArray "object" methods (function pointers)

int append(DynamicArray* self, int value){
    if (self->len == self->capacity){
        self->capacity *= 2;
        if (resize_array(self, self->capacity) < 0){
            return DA_ERROR_ALLOC;
        }
    }
    self->array[self->len++] = value;

    return;
}


int insert(DynamicArray* self, int index, int value){
    /*
    Validations:
        - insert index out of range
        - insert at the end
        - insert at the beginning
        - insert in the middle
    */
    if (index >= self->len || index < 0){
        return DA_ERROR_INDEX;
    }

    // Insert in the beginning
    if (index == 0){
        for (int i=self->len; i>0; i--){
            self->array[i] = self->array[i-1];
        }
        self->array[0] = value;
        return;
    }

    // Insert in the end
    else if (index == self->len){
        if (self->len == self->capacity){
            self->capacity *= 2;
            if (resize_array(self, self->capacity) < 0){
                return DA_ERROR_ALLOC;
            }
        }
        self->array[self->len++] = value;
        return;
    }

    // Insert in the middle
    else{
        for (int i=self->len; i>index; i--){
            if (self->len == self->capacity){
                self->capacity *= 2;
                int is_resized = resize_array(self, self->capacity);
                if (is_resized < 0){
                    perror("Failed to resize array: DynamicArray::insert()");
                    exit(1);
                }
            }
            self->array[i] == self->array[i-1];
        }
        self->array[index] = value;

        self->len++;
        return;
    }
}


int pop(DynamicArray* self){
    int removed_element = self->array[--self->len];
    self->array[self->len] = 0;

    if (self->len == self->capacity * 0.25){
        self->capacity /= 2;
        if (resize_array(self, self->capacity) < 0){
            return DA_ERROR_ALLOC;
        }
    }
    
    return removed_element;
}


int remove(DynamicArray* self, int index){
    // Very basic implementation
    int removed_element = self->array[index];
    for (int i=index; i<self->len; i++){
        self->array[i] = self->array[i+1];
    }
    self->array[--self->len] = 0;

    return removed_element;
}


// Constructor function

DynamicArray dynamic_array_init(int capacity){
    DynamicArray array;
    array.array = malloc(capacity * sizeof(int));
    array.len = 0;
    array.capacity = capacity ? capacity >= 2 : 2;

    // So-called "methods" of DynamicArray struct
    array.append = append;
    array.insert = insert;
    array.pop = pop;
    array.remove = remove;

    return array;
}
