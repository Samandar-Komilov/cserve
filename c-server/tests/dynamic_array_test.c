#include <check.h>
#include "../src/DataStructures/dynamic_array.h"

START_TEST(test_append)
{
    DynamicArray arr = dynamic_array_init(1);
    arr.append(&arr, 100);
    ck_assert_int_eq(arr.array[0], 100);
}
END_TEST


Suite* dynamic_array_suite(void) {
    Suite *s = suite_create("DynamicArray");
    TCase *tc = tcase_create("Core");
    tcase_add_test(tc, test_append);
    suite_add_tcase(s, tc);
    return s;
}
