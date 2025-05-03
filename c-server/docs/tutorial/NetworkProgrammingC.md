# Reference: Beej's Guide to Network Programming

> I understood that I have to learn network programming deeper to go further. So, let's read Beej's Guide to Network Programming together.

## Chapter 2: Sockets
In very raw terms, socket is a "door" to a process. A process can communicate with outside world (other processes) through sockets. But what is it under the hood? Well, they’re this: a way to speak to other programs using standard Unix file descriptors!
We know that everything is a file in Unix-like systems. A file descriptor is just an integer pointing to that particular open "file": not only a file, but a connection, a pipe, a terminal, whatever.

### `socket()` syscall
`socket()` creates an endpoint for communication and returns a descriptor.
```c
int socket(int domain, int type, int protocol);
```
The `domain` argument specifies a communication domain; this selects the protocol family which will be used for communication.
```c
#define AF_UNSPEC       0       /* unspecified */
#define AF_UNIX         1       /* local to host (pipes, portals) */
#define AF_INET         2       /* IPv4 */
#define AF_INET6        2       /* IPv6 */
#define AF_NETLINK      3       /* Kernel user interface device */
#define AF_PUP          4       /* pup protocols: e.g. BSP */
#define AF_CHAOS        5       /* mit chaos protocols */
```

The `type` argument specifies the communication semantics; this selects the socket type.
```c
#define SOCK_STREAM     1       /* stream socket */
#define SOCK_DGRAM      2       /* datagram socket */
#define SOCK_RAW        3       /* raw-protocol interface */
#define SOCK_RDM        4       /* reliably-delivered message */
#define SOCK_SEQPACKET  5       /* sequenced packet stream */
```

### 2 Types of Sockets

**Stream sockets**: TCP. They are reliable two-way connected communication streams. If you output two items into the socket in the order “1, 2”, they will arrive in the order “1, 2” at the opposite end.
- **Datagram socket**: UDP. They are error-prone, connectionless, unreliable messages. They are not guaranteed to arrive in the same order as sent.



## Chapter 3: IP addresses, structs and Data Munging

Well, in short, there are 2 types of IP addresses:
- IPv4 (like `127.0.0.1`)
- IPv6 (like `2b5b:1e49:8d01:c2ac:fffd:833e:dfee:13a4`)

### Subnets
> Soon, I'll investigate further and write about subnets.

### Port Numbers
While the IP address is like a "street address" of the computer, the port number is like a "door number" - which process responds to the request on which number.
Why? Let's say your computer is running Redis and FastAPI backend services at the same time. How you can access FastAPI service exactly? Both of them are 127.0.0.1! So, ports are to rescue: 6479 is for Redis, 8000 is for FastAPI.

### Byte Order
The thing is, everyone in the Internet world has generally agreed that if you want to represent the two-byte hex number, say `b34f`, you’ll store it in two sequential bytes `b3` followed by `4f`. This number, stored with the big end first is called **Big Endian**:
```
b3 4f
```
Unfortunately, any Intel or Intel related computer scattered this and stores bytes in reverse order:
```
4f b3
```
This is called the **Little Endian**. (What the heck? The same number written in reverse order.)

Your computer stores bytes in its host order. So, we have to be sure while sending bytes over the network: they are in Big Endian! But how to do this?

```c
htons(uint16_t hostshort) // host to network short
htonl(uint32_t hostlong) // host to network long
ntohs(uint16_t netshort) // network to host short
ntohl(uint32_t netlong) // network to host long
```
See man page for more help. (e.g. `man htons`)

### Structs
So, let's start figuring out what all of the things we discussed are actually implemented in C (internally in the kernel) using system calls.

**Socket Descriptor** - well, simply `int`. 
**Socket Preparation** - `struct addrinfo`. This structure is a more recent invention, and is used to prep the socket address structures for subsequent use. It’s also used in host name lookups, and service name lookups. That’ll make more sense later when we get to actual usage, but just know for now that it’s one of the first things you’ll call when making a connection.
```c
struct addrinfo {
    int ai_flags;              // AI_PASSIVE, AI_CANONNAME, etc.
    int ai_family;             // AF_INET, AF_INET6, AF_UNSPEC
    int ai_socktype;           // SOCK_STREAM, SOCK_DGRAM
    int ai_protocol;           // IPPROTO_TCP, IPPROTO_UDP (0 for any)
    size_t ai_addrlen;         // length of ai_addr
    char *ai_canonname;        // canonical name for service location

    struct sockaddr *ai_addr;  // struct sockaddr_in: pointer to socket address
    struct addrinfo *ai_next;  // next struct addrinfo (linked list)
};
```

