/**
 * @file    test_runner.c
 * @author  Samandar Komil
 * @date    22 April 2025
 * @brief   Main Test Runners
 *
 */

#include <check.h>

Suite *http_parser_suite(void);

int main(void)
{
    int failed  = 0;
    SRunner *sr = srunner_create(http_parser_suite());

    srunner_run_all(sr, CK_NORMAL);
    failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (failed == 0) ? 0 : 1;
}
