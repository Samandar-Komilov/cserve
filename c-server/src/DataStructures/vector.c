/**
 * @file    vector.c
 * @author  Samandar Komil
 * @date    19 April 2025
 * @brief   Defines the Vector struct and its methods
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "vector.h"
#include "../common.h"

// Utility functions
int resize_array(Vector *self, int new_capacity)
{
    if (new_capacity < MIN_CAPACITY) return VECTOR_ALLOC_ERROR;
    int *temp_arr = realloc(self->array, new_capacity * sizeof(int));
    if (!temp_arr) return VECTOR_ALLOC_ERROR;
    self->array    = temp_arr;
    self->capacity = new_capacity;
    return VECTOR_SUCCESS;
}

// Vector "object" methods (function pointers)

int append(Vector *self, int value)
{
    if (!self) return VECTOR_NULLPTR_ERROR;
    if (self->capacity < MIN_CAPACITY) return VECTOR_ALLOC_ERROR;

    if (self->len == self->capacity)
    {
        self->capacity *= 2;
        if (resize_array(self, self->capacity) < 0)
        {
            return VECTOR_ALLOC_ERROR;
        }
    }
    self->array[self->len++] = value;

    return VECTOR_SUCCESS;
}

int insert(Vector *self, int index, int value)
{
    if (!self) return VECTOR_NULLPTR_ERROR;

    if (index > self->len || index < 0)
    {
        return VECTOR_INDEX_ERROR;
    }

    // Insert in the beginning
    if (index == 0)
    {
        for (int i = self->len; i > 0; i--)
        {
            self->array[i] = self->array[i - 1];
        }
        self->array[0] = value;
        return VECTOR_SUCCESS;
    }

    // Insert in the end
    else if (index == self->len)
    {
        if (self->len == self->capacity)
        {
            self->capacity *= 2;
            if (resize_array(self, self->capacity) < 0)
            {
                return VECTOR_ALLOC_ERROR;
            }
        }
        self->array[self->len++] = value;
        return VECTOR_SUCCESS;
    }

    // Insert in the middle
    else
    {
        for (int i = self->len; i > index; i--)
        {
            if (self->len == self->capacity)
            {
                self->capacity *= 2;
                if (resize_array(self, self->capacity) < 0)
                {
                    return VECTOR_ALLOC_ERROR;
                }
            }
            self->array[i] = self->array[i - 1];
        }
        self->array[index] = value;

        self->len++;
        return VECTOR_SUCCESS;
    }
}

int pop(Vector *self)
{
    if (!self) return VECTOR_NULLPTR_ERROR;
    int removed_element    = self->array[--self->len];
    self->array[self->len] = 0;

    if (self->len == self->capacity * 0.25 && self->len > MIN_CAPACITY)
    {
        self->capacity /= 2;
        if (resize_array(self, self->capacity) < 0)
        {
            return VECTOR_ALLOC_ERROR;
        }
    }

    return removed_element;
}

int pop_at(Vector *self, int index)
{
    if (!self) return VECTOR_NULLPTR_ERROR;
    int removed_element = self->array[index];
    for (int i = index; i < self->len; i++)
    {
        self->array[i] = self->array[i + 1];
    }
    self->array[--self->len] = 0;

    return removed_element;
}

void list_elements(Vector *self)
{
    for (int i = 0; i < self->len; i++)
    {
        printf("%d ", self->array[i]);
    }
    printf("\n");
}

// Constructor function

Vector vector_init(int capacity)
{
    Vector array;
    array.array    = malloc(capacity * sizeof(int));
    array.len      = 0;
    array.capacity = (capacity >= 2) ? capacity : 2;

    // So-called "methods" of Vector struct
    array.append        = append;
    array.insert        = insert;
    array.pop           = pop;
    array.pop_at        = pop_at;
    array.list_elements = list_elements;

    return array;
}

int vector_free(Vector *self)
{
    if (!self) return VECTOR_NULLPTR_ERROR;
    free(self->array);
    return 0;
}