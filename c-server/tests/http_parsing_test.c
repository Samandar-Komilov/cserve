/**
 * @file    http_request_response.c
 * @author  Samandar Komil
 * @date    25 April 2025
 * @brief   Tests for http/server.c, request.c and response.c implementations
 *
 */

#include <check.h>
#include "common.h"
#include "http/server.h"
#include "http/parsers.h"

HTTPRequest *req;

void setup(void)
{
    req = httprequest_constructor();
}

void teardown(void)
{
    httprequest_free(req);
}

START_TEST(test_parse_request_line_valid)
{
    char line[]  = "GET /index.html HTTP/1.1";
    int res_code = parse_request_line(req, line);

    ck_assert_str_eq(req->method, "GET");
    ck_assert_str_eq(req->path, "/index.html");
    ck_assert_str_eq(req->version, "HTTP/1.1");
    ck_assert_int_eq(res_code, OK);
}
END_TEST

START_TEST(test_parse_request_line_invalid)
{
    char line1[] = "INVALID / HTTP/1.1";
    char line2[] = "GET INVALID HTTP/1.1";
    char line3[] = "INVALID_WHATEVER";

    int res_code_method  = parse_request_line(req, line1);
    int res_code_path    = parse_request_line(req, line2);
    int res_code_invalid = parse_request_line(req, line3);

    ck_assert_int_eq(res_code_method, INVALID_METHOD);
    ck_assert_int_eq(res_code_path, INVALID_PATH);
    ck_assert_int_eq(res_code_invalid, INVALID_REQUEST_LINE);
}
END_TEST

START_TEST(test_parse_headers_single)
{
    char headers[] = "Host: localhost\r\n";
    parse_headers(req, headers);

    ck_assert_int_eq(req->header_count, 1);
    ck_assert_str_eq(req->headers[0].key, "Host");
    ck_assert_str_eq(req->headers[0].value, "localhost");
}
END_TEST

START_TEST(test_parse_headers_multiple)
{
    char headers[] = "Host: localhost\r\nContent-Type: text/html\r\n";
    parse_headers(req, headers);

    ck_assert_int_eq(req->header_count, 2);
    ck_assert_str_eq(req->headers[1].key, "Content-Type");
    ck_assert_str_eq(req->headers[1].value, "text/html");
}
END_TEST

START_TEST(test_parse_headers_colon_missing)
{
    char headers[] = "InvalidHeader\r\n";
    parse_headers(req, headers);

    ck_assert_int_eq(req->header_count, 0);
}
END_TEST

START_TEST(test_parse_body_non_empty)
{
    char *body = "This is the body.";
    parse_body(req, body, strlen(body));

    ck_assert_str_eq(req->body, "This is the body.");
    ck_assert_int_eq(req->body_length, strlen(body));
}
END_TEST

START_TEST(test_parse_body_empty)
{
    parse_body(req, NULL, 0);

    ck_assert_ptr_null(req->body);
    ck_assert_int_eq(req->body_length, 0);
}
END_TEST

Suite *http_parser_suite(void)
{
    Suite *s       = suite_create("HTTP Parser");
    TCase *tc_core = tcase_create("Core");

    tcase_add_checked_fixture(tc_core, setup, teardown);

    tcase_add_test(tc_core, test_parse_request_line_valid);
    tcase_add_test(tc_core, test_parse_request_line_invalid);
    tcase_add_test(tc_core, test_parse_headers_single);
    tcase_add_test(tc_core, test_parse_headers_multiple);
    tcase_add_test(tc_core, test_parse_headers_colon_missing);
    tcase_add_test(tc_core, test_parse_body_non_empty);
    tcase_add_test(tc_core, test_parse_body_empty);

    suite_add_tcase(s, tc_core);
    return s;
}