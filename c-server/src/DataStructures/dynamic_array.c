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
#include <stdio.h>
#include "dynamic_array.h"

// Utility functions
int resize_array(DynamicArray *self, int new_capacity)
{
    int *temp_arr = realloc(self->array, new_capacity * sizeof(int));
    if (!temp_arr) return DA_ERROR_ALLOC;
    self->array    = temp_arr;
    self->capacity = new_capacity;
    return DA_SUCCESS;
}

// DynamicArray "object" methods (function pointers)

int append(DynamicArray *self, int value)
{
    if (!self) return DA_ERROR_NULLPTR;

    if (self->len == self->capacity)
    {
        self->capacity *= 2;
        if (resize_array(self, self->capacity) < 0)
        {
            return DA_ERROR_ALLOC;
        }
    }
    self->array[self->len++] = value;

    return DA_SUCCESS;
}

int insert(DynamicArray *self, int index, int value)
{
    if (!self) return DA_ERROR_NULLPTR;

    if (index > self->len || index < 0)
    {
        return DA_ERROR_INDEX;
    }

    // Insert in the beginning
    if (index == 0)
    {
        for (int i = self->len; i > 0; i--)
        {
            self->array[i] = self->array[i - 1];
        }
        self->array[0] = value;
        return DA_SUCCESS;
    }

    // Insert in the end
    else if (index == self->len)
    {
        if (self->len == self->capacity)
        {
            self->capacity *= 2;
            if (resize_array(self, self->capacity) < 0)
            {
                return DA_ERROR_ALLOC;
            }
        }
        self->array[self->len++] = value;
        return DA_SUCCESS;
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
                    return DA_ERROR_ALLOC;
                }
            }
            self->array[i] = self->array[i - 1];
        }
        self->array[index] = value;

        self->len++;
        return DA_SUCCESS;
    }
}

int pop(DynamicArray *self)
{
    if (!self) return DA_ERROR_NULLPTR;
    int removed_element    = self->array[--self->len];
    self->array[self->len] = 0;

    if (self->len == self->capacity * 0.25)
    {
        self->capacity /= 2;
        if (resize_array(self, self->capacity) < 0)
        {
            return DA_ERROR_ALLOC;
        }
    }

    return removed_element;
}

int pop_at(DynamicArray *self, int index)
{
    if (!self) return DA_ERROR_NULLPTR;
    int removed_element = self->array[index];
    for (int i = index; i < self->len; i++)
    {
        self->array[i] = self->array[i + 1];
    }
    self->array[--self->len] = 0;

    return removed_element;
}

void list_elements(DynamicArray *self)
{
    for (int i = 0; i < self->len; i++)
    {
        printf("%d ", self->array[i]);
    }
    printf("\n");
}

// Constructor function

DynamicArray dynamic_array_init(int capacity)
{
    DynamicArray array;
    array.array    = malloc(capacity * sizeof(int));
    array.len      = 0;
    array.capacity = (capacity >= 2) ? capacity : 2;

    // So-called "methods" of DynamicArray struct
    array.append        = append;
    array.insert        = insert;
    array.pop           = pop;
    array.pop_at        = pop_at;
    array.list_elements = list_elements;

    return array;
}

int dynamic_array_free(DynamicArray *self)
{
    if (!self) return DA_ERROR_NULLPTR;
    free(self->array);
    return 0;
}