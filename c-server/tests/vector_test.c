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

// START_TEST(test_resize_array_success)
// {
//     Vector arr       = vector_init(1);
//     int new_capacity = 2;
//     int result       = resize_array(&arr, new_capacity);
//     ck_assert_int_eq(result, VECTOR_SUCCESS);
//     ck_assert_int_eq(arr.capacity, new_capacity);
//     vector_free(&arr);
// }

// Suite *vector_suite(void)
// {
//     Suite *s  = suite_create("Vector");
//     TCase *tc = tcase_create("Core");
//     tcase_add_test(tc, test_resize_array_success);
//     suite_add_tcase(s, tc);
//     return s;
// }