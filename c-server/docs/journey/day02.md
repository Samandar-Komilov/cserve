# Day 2 - April 18, 2025

Finally, today I completed my solid plan through a version-based approach, with the help of Grok. Yes, I used AI :).

> There was a simple introductory TCP server implementation, written while I was watching Eric Meehan's video series. I let it stay for now, and started building my own `DynamicArray`, as I use it a lot.

I expected it to be easy, but... I forgot many things, that's the effect of working in Python only in last months definitely. So, I decided to start from scratch.

1. **Building DynamicArray - Trial 1**
Here, I simply defined a struct with `data`, `length` and `capacity` fields, data holding only integers. Certainly for simplicity. Before making everything complex, I tried implementing `append()`, `insert()`, `pop()` and similar methods using function pointers. I wrote very basic implementations for each, not checking for edge cases even. Because I first wanted to setup Test framework and valgrind to start using.
Check framework worked as expected, I wrote a sample test and ran. Valgrind, as always, showed that I'm leaking memory of `4n` bytes while mallocing the dynamic array initialization.
Then I changed my file structure, added future `rust-server` README and wrote appropriate `Makefile`. This took the day 2.