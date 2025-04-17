# Day 1 - April 17, 2025

> The journey begins. I was so motivated. I had a reference video on building a HTTP server. I was thinking I know C well still, but...

Well, the day really confused me. First, I watched several videos of Eric Meehan's playlist, however then realized he is not building what I was looking for. Plus, the implementation were a bit complex - the usage of linkedlist with binary search tree to parse HTTP, that's overkill. As it's not enough, I forgot to work with pointers in C🤦‍♂️. What the... Thanks to Beej's Guide to C book, which helped me to refresh my knowledge quickly (on a bus:).

So, I decided not to rush. The initial thing was to make clear - **what I want to build?** Gunicorn-like Python web server? Or Nginx like web server? What differentiates them? 
Such questions really made me stuck and demotivated. But then I forgot about everything and restarted to think from scratch. Researched a bit and found that I'm building NGINX-like web server!

Great, as now I have a clear vision, the only thing needed was a solid plan with toolkit. My plan became version-based:
**- V0. Basic TCP Server**
**- V1. Basic HTTP Server**
**- V2. Static File Serving**
**- V3. Reverse Proxy**
**- V4. Load Balancing**
**- V5. Config File (.ini)**
**- V6. Concurrency (Event-based)**

I knew that it is not simply writing code at night, I also should analyze every corner, test the code, benchmark it and use best practices. So, I made a toolkit for me too:
- [Valgrind](https://valgrind.org/docs/manual/quick-start.html) for memory leak checks
- **GDB** debugger to debug runtime issues
- [Check](https://libcheck.github.io/check/) framework for Unit Testing
- [wrk](https://github.com/wg/wrk) for modern benchmarking
- Custom logging and tracing

> The whole day spent to build a solid plan. Well, let's move onto Day 2!