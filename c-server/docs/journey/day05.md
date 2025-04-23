# Day 5 - April 23, 2025

Finally, we came into parsing the actual HTTP request. I have several options for tokenizing the request, including `strtok()`, `strchr()`. We'll try to benefit from both. [01:34]

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
3. The body [17:05]

So, we write separate parsers for each, and then glue them together in a final `parse_http_request()` function. Before that, I had to make sure `strtok()` and `strchr()` can help me. Hence, I tested the following, ensuring `strtok()` can tokenize the overall request into lines, so that I can parse further:
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

Great. As I expected. So, I continued implementing my HTTP request parser like this, by separating each 3 parts into separate functions:

```c
HTTPServer *parse_http_request(HTTPServer *httpserver_ptr, char *request)
{
    char *token = strtok(request, "\r\n");

    // 1. Request Line
    parse_request_line(httpserver_ptr, token);
    token = strtok(NULL, "\r\n");

    // 2. Headers
    parse_headers(httpserver_ptr, token);
    token = strtok(NULL, "\r\n");

    // 3. Body
    parse_body(httpserver_ptr, token);

    return httpserver_ptr;
}
```

So, my initial implementation for parsing request line became as follows:
```c
HTTPServer *parse_request_line(HTTPServer *httpserver_ptr, char *request_line)
{
    char *method  = (char *)malloc(10 * sizeof(char));
    char *path    = (char *)malloc(100 * sizeof(char));
    char *version = (char *)malloc(10 * sizeof(char));

    // 1. HTTP request type
    char *token = strtok(request_line, " ");
    strcpy(method, token);

    // 2. HTTP request path (we need splits again)
    token = strtok(NULL, " ");
    strcpy(path, token);
    // parse_request_path(httpserver, token);

    // 3. HTTP version
    token = strtok(NULL, " ");
    strcpy(version, token);

    printf("Request Line:\n%s %s %s\n", method, path, version);

    httpserver_ptr->request[0] = method;
    httpserver_ptr->request[1] = path;
    httpserver_ptr->request[2] = version;

    return httpserver_ptr;
}
```

And it worked, according to my manual tests. But...

### Design Issue

I sensed a huge mistake here. Why am I saving all of the request data right in the HTTPServer struct fields? How does it make sense? Each incoming request should be HTTPRequest construct and every response should be HTTPResponse construct. Okay for blocking connections with `while` loop and `accept()`, but the flow should still continue correctly. Ahh... [00:47]