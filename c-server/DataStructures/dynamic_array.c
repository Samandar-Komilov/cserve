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


// DynamicArray "object" methods (function pointers)

void append(DynamicArray* self, int value){
    // Very basic implementation
    self->array[self->len++] = value;

    return;
}


void insert(DynamicArray* self, int index, int value){
    // Very basic implementation
    for (int i=0; i<self->len; i++){
        if (i == index){
            self->array[i] == value;
            return;
        }
    }
    return;
}


int pop(DynamicArray* self){
    // Very basic implementation
    int removed_element = self->array[--self->len];
    self->array[self->len] = NULL;
    
    return removed_element;
}


int remove(DynamicArray* self, int index){
    // Very basic implementation
    int removed_element = self->array[index];
    for (int i=index; i<self->len; i++){
        self->array[i] = self->array[i+1];
    }
    self->array[--self->len] = NULL;

    return removed_element;
}


// Constructor function

DynamicArray dynamic_array_init(int capacity){
    DynamicArray array;
    array.array = malloc(capacity * sizeof(int));
    array.len = 0;
    array.capacity = capacity;

    // Function pointer assignments
    array.append = append;
    array.insert = insert;
    array.pop = pop;
    array.remove = remove;

    return array;
}
