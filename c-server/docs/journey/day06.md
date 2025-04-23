# Day 6 - April 24, 2025

> Okay, let's plan first. 

Here is the [mermaid](https://mermaid.liveedit.me/) diagram of the process:

```mermaid
sequenceDiagram
    participant Client as "Client"
    participant HTTPServer as "HTTPServer"
    participant Parser as "Parser"
    participant HTTPRequest as "HTTPRequest"
    participant Handler as "Handler Function"
    participant HTTPResponse as "HTTPResponse"

    Note over Client,HTTPServer: Client sends request
    Client->>HTTPServer: Request
    HTTPServer->>Parser: Parse request
    Parser->>HTTPRequest: Save to HTTPRequest
    HTTPRequest->>Handler: Throw to handler function
    Handler->>HTTPResponse: Return HTTPResponse
    HTTPResponse->>Client: Send response back to client
    Note over HTTPServer: HTTPServer returns response
```

So, it is much clear right now that we messed up HTTPServer and HTTPRequest stages previously. Right now we have to fix this first.

1. Define `HTTPRequest` struct
2. Refactor `HTTPServer` struct
3. Refactor `parse_http_request()`
4. Add memory safety best-practices:
    - centralized alloc-dealloc
    - signal handlers and standard error message codes

So, we know our next steps, let's start. [00:59]

