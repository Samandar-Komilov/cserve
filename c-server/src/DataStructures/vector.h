/**
 * @file    vector.h
 * @author  Samandar Komil
 * @date    18 April 2025
 * @brief   Vector construct definitions and function prototypes
 *
 */

#define MIN_CAPACITY 2

typedef struct Vector
{
    int *array;
    int len;
    int capacity;

    int (*append)(struct Vector *self, int value);
    int (*insert)(struct Vector *self, int index, int value);
    int (*pop)(struct Vector *self);
    int (*pop_at)(struct Vector *self, int index);
    void (*list_elements)(struct Vector *self);
} Vector;

// Prototypes

int resize_array(Vector *self, int new_capacity);

Vector vector_init(int capacity);
int vector_free(Vector *self);

int append(Vector *self, int value);
int insert(Vector *self, int index, int value);
int pop(Vector *self);
int pop_at(Vector *self, int index);
void list_elements(Vector *self);