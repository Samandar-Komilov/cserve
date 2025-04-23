/**
 * @file    vector_test.c
 * @author  Samandar Komil
 * @date    23 April 2025
 * @brief   Tests for vector.c
 *
 */

#include <check.h>
#include "common.h"
#include "../src/DataStructures/vector.h"

START_TEST(test_resize_array_success)
{
    Vector arr       = vector_init(1);
    int new_capacity = 2;
    int result       = resize_array(&arr, new_capacity);
    ck_assert_int_eq(result, DA_SUCCESS);
    ck_assert_int_eq(arr.capacity, new_capacity);
    vector_free(&arr);
}
END_TEST

START_TEST(test_resize_array_failure)
{
    Vector arr       = vector_init(1);
    int new_capacity = 0;
    int result       = resize_array(&arr, new_capacity);
    ck_assert_int_eq(result, DA_ERROR_ALLOC);
    vector_free(&arr);
}
END_TEST

START_TEST(test_append_success)
{
    Vector arr = vector_init(1);
    int value  = 100;
    int result = arr.append(&arr, value);
    ck_assert_int_eq(result, DA_SUCCESS);
    ck_assert_int_eq(arr.array[0], value);
    vector_free(&arr);
}
END_TEST

START_TEST(test_append_failure_nullptr)
{
    Vector arr = vector_init(1);
    int value  = 100;
    int result = arr.append(NULL, value);
    ck_assert_int_eq(result, DA_ERROR_NULLPTR);
    vector_free(&arr);
}
END_TEST

START_TEST(test_append_failure_alloc)
{
    Vector arr = vector_init(1);
    int value  = 100;
    // Simulate allocation failure
    arr.capacity = 0;
    int result   = arr.append(&arr, value);
    ck_assert_int_eq(result, DA_ERROR_ALLOC);
    vector_free(&arr);
}
END_TEST

START_TEST(test_insert_success_beginning)
{
    Vector arr = vector_init(1);
    int value  = 100;
    int index  = 0;
    int result = arr.insert(&arr, index, value);
    ck_assert_int_eq(result, DA_SUCCESS);
    ck_assert_int_eq(arr.array[0], value);
    vector_free(&arr);
}
END_TEST

START_TEST(test_insert_success_end)
{
    Vector arr = vector_init(1);
    int value  = 100;
    int index  = 1;
    arr.append(&arr, 50);
    int result = arr.insert(&arr, index, value);
    ck_assert_int_eq(result, DA_SUCCESS);
    ck_assert_int_eq(arr.array[1], value);
    vector_free(&arr);
}
END_TEST

START_TEST(test_insert_success_middle)
{
    Vector arr = vector_init(2);
    int value  = 100;
    int index  = 1;
    // arr.append(&arr, 50);
    int result = arr.insert(&arr, index, value);
    ck_assert_int_eq(result, DA_ERROR_INDEX);
    vector_free(&arr);
}
END_TEST

START_TEST(test_insert_failure_nullptr)
{
    Vector arr = vector_init(1);
    int value  = 100;
    int index  = 0;
    int result = arr.insert(NULL, index, value);
    ck_assert_int_eq(result, DA_ERROR_NULLPTR);
    vector_free(&arr);
}
END_TEST

START_TEST(test_insert_failure_index)
{
    Vector arr = vector_init(1);
    int value  = 100;
    int index  = 2;
    int result = arr.insert(&arr, index, value);
    ck_assert_int_eq(result, DA_ERROR_INDEX);
    vector_free(&arr);
}
END_TEST

START_TEST(test_pop_success)
{
    Vector arr = vector_init(1);
    int value  = 100;
    append(&arr, value);
    int result = arr.pop(&arr);
    ck_assert_int_eq(result, value);
    ck_assert_int_eq(arr.len, 0);
    vector_free(&arr);
}
END_TEST

START_TEST(test_pop_failure_nullptr)
{
    Vector arr = vector_init(1);
    int result = arr.pop(NULL);
    ck_assert_int_eq(result, DA_ERROR_NULLPTR);
    vector_free(&arr);
}
END_TEST

START_TEST(test_pop_at_success)
{
    Vector arr = vector_init(2);
    int value1 = 100;
    int value2 = 200;
    append(&arr, value1);
    append(&arr, value2);
    int result = arr.pop_at(&arr, 0);
    ck_assert_int_eq(result, value1);
    ck_assert_int_eq(arr.len, 1);
    vector_free(&arr);
}
END_TEST

START_TEST(test_pop_at_failure_nullptr)
{
    Vector arr = vector_init(1);
    int result = arr.pop_at(NULL, 0);
    ck_assert_int_eq(result, DA_ERROR_NULLPTR);
    vector_free(&arr);
}
END_TEST

Suite *vector_suite(void)
{
    Suite *s  = suite_create("Vector");
    TCase *tc = tcase_create("Core");
    tcase_add_test(tc, test_resize_array_success);
    tcase_add_test(tc, test_resize_array_failure);
    tcase_add_test(tc, test_append_success);
    tcase_add_test(tc, test_append_failure_alloc);
    tcase_add_test(tc, test_append_failure_nullptr);
    tcase_add_test(tc, test_insert_failure_index);
    tcase_add_test(tc, test_insert_failure_nullptr);
    tcase_add_test(tc, test_insert_success_beginning);
    tcase_add_test(tc, test_insert_success_end);
    tcase_add_test(tc, test_insert_success_middle);
    tcase_add_test(tc, test_pop_success);
    tcase_add_test(tc, test_pop_failure_nullptr);
    tcase_add_test(tc, test_pop_at_success);
    tcase_add_test(tc, test_pop_at_failure_nullptr);
    suite_add_tcase(s, tc);
    return s;
}