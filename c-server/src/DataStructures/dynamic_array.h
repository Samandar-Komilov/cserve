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

#define MIN_CAPACITY 2

enum
{
    DA_SUCCESS       = 0,
    DA_ERROR_ALLOC   = -1,
    DA_ERROR_INDEX   = -2,
    DA_ERROR_NULLPTR = -3
};

typedef struct DynamicArray
{
    int *array;
    int len;
    int capacity;

    int (*append)(struct DynamicArray *self, int value);
    int (*insert)(struct DynamicArray *self, int index, int value);
    int (*pop)(struct DynamicArray *self);
    int (*pop_at)(struct DynamicArray *self, int index);
    void (*list_elements)(struct DynamicArray *self);
} DynamicArray;

// Prototypes

int resize_array(DynamicArray *self, int new_capacity);

DynamicArray dynamic_array_init(int capacity);
int dynamic_array_free(DynamicArray *self);

int append(DynamicArray *self, int value);
int insert(DynamicArray *self, int index, int value);
int pop(DynamicArray *self);
int pop_at(DynamicArray *self, int index);
void list_elements(DynamicArray *self);