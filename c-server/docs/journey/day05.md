# Day 3 - April 23, 2025

Finally, we came into parsing the actual HTTP request. I have several options for tokenizing the request, including `strtok()`, `strchr()`. We'll try to benefit from both.

The general HTTP request format is:

```
GET /index.html HTTP/1.1
Host: example.com
User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:52.0) Gecko/20100101 Firefox/52.0
Accept: text/html,application/json
Accept-Language: en-US,en;q=0.5
Accept-Encoding: gzip, deflate

{
    "username": "admin",
    "password": "password"
}
```

We can see the request consists of 3 main parts:

1. The request line
2. The headers
3. The body

So, we write separate parsers for each, and then glue them together in a final `http_request_parser()` function.


Tested the following, ensuring `strtok()` can tokenize the overall request into lines, so that I can parse further:
```c
char teststr[500] =
        "GET /index.html HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:52.0) Gecko/20100101 Firefox/52.0\r\n"
        "Accept: text/html,application/json\r\n"
        "Accept-Language: en-US,en;q=0.5\r\n"
        "Accept-Encoding: gzip, deflate\r\n";
char *tokptr = strtok(teststr, "\r\n");

while (tokptr != NULL)
{
    printf("%s\n", tokptr);
    tokptr = strtok(NULL, "\r\n");
}
```

The again tested to parse the request line itself with `strtok()`:
```c
char teststr[100] = "GET /index.html HTTP/1.1";
char *tokptr      = strtok(teststr, " ");

while (tokptr != NULL)
{
    printf("%s\n", tokptr);
    tokptr = strtok(NULL, " ");
}
```