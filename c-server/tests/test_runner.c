/**
 * @file    test_runner.c
 * @author  Samandar Komil
 * @date    22 April 2025
 * @brief   Main Test Runners
 *
 */

#include <check.h>

Suite *vector_suite(void);
// Suite* server_suite(void);  <-- for future

int main(void)
{
    int failed  = 0;
    SRunner *sr = srunner_create(vector_suite());

    // srunner_add_suite(sr, server_suite());  <-- add more as you grow

    srunner_run_all(sr, CK_NORMAL);
    failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (failed == 0) ? 0 : 1;
}
