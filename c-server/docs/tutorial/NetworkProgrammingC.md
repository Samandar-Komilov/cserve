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
**Socket Preparation** - `struct addrinfo`. This structure is a more recent invention, and is used to prepare the socket address structures for subsequent use. It’s also used in host name lookups, and service name lookups. That’ll make more sense later when we get to actual usage, but just know for now that it’s one of the first things you’ll call when making a connection.
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
**Socket Address Information Generic** - `struct sockaddr`. This structure is used as a generic structure to hold various types of sockets. It only has 2 members: address family and 14 bytes protocol address:
```c
struct sockaddr {
    unsigned short sa_family; // AF_INET, AF_INET6, AF_UNSPEC
    char sa_data[14];         // protocol address
}
```
**IPv4 and IPv6 specific structures** - `struct sockaddr_in` and `struct sockaddr_in6`. The reason why we need a generic type is `connect()` expects a IP version-agnostic type. And the reason why we need specific structures per version, for simplicity.
```c
struct sockaddr_in {
    short int sin_family;
    unsigned short int sin_port;
    struct in_addr sin_addr;
    char sin_zero[8];
};

struct sockaddr_in6 {
    short int sin6_family;
    unsigned short int sin6_port;
    unsigned int sin6_flowinfo;
    struct in6_addr sin6_addr;
    unsigned int sin6_scope_id;
};
```

> The only thing we need to memorize is, these structures are crucial in working with sockets and holding their information efficiently. Otherwise we would determine each small detail and build struct ourselves.

---

## Chapter 5: Socket System Calls
This is a section where we introduce a number of system calls to work with sockets:
```c
int getaddrinfo();

int socket();

int bind();
int connect();
int listen();
int accept();

int send();
int recv();
close();
int shutdown();

int getpeername();
int gethostname();
```

### `getaddrinfo()` - Prepare to Launch!
In ancient times, we had to manually populate `struct sockaddr_in` structure by calling `gethostbyname()` for DNS lookups. But this is no more necessary. This system call performs everything for us:
```c
int getaddrinfo(
    const char *restrict node,              // is the host name to connect to, or an IP address.
    const char *restrict service,           // port number (or other service name)
    const struct addrinfo *restrict hints,  // points to struct addrinfo
    struct addrinfo **restrict res          // 
);
```
**So, anyway what is `getaddrinfo`?**  
`getaddrinfo()` is a C library function that helps you convert a human-readable address (like "google.com" or "localhost") and a port (like "80" or "http") into a list of structures you can use to create a socket and `connect`/`bind`. It's like:
> I have my website domain name and port, and I want to connect it. Give all low-level structs to me without having me trouble.
Basic Usage:
```c
struct addrinfo hints, *res;
memset(&hints, 0, sizeof hints);
hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
hints.ai_socktype = SOCK_STREAM; // TCP

getaddrinfo("example.com", "80", &hints, &res);

// Now `res` points to a list of possible addresses you can connect() or bind() to.
// You can use `socket()` + `connect()` with the info from `res`.

// When done:
freeaddrinfo(res);
```
❓ What does `res` contain? A linked list of `struct addrinfo`, where each node contains socket address information, linked to the provided name. You loop over the list and try one until it works.


### `socket()` - Get the File Descriptor
This system call gives you a socket descriptor based on your provided domain, type of socket and protocol:
```c
int socket(
    int domain,     // AF_INET or AF_INET6
    int type,       // SOCK_STREAM or SOCK_DGRAM
    int protocol    // IPPROTO_TCP or IPPROTO_UDP
);
```
So, what we really need is to take result from `getaddrinfo()` and feed `socket()` with it directly:
```c
int s;
struct addrinfo hints, *res;

getaddrinfo("example.com", "80", &hints, &res);

s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
```


### `bind()` - What port am I on?
Once you have a socket, if you want to listen for connections, you need to bind it to a port on your local machine. This system call does the job for you:
```c
int bind(
    int sockfd,                 // socket descriptor
    struct sockaddr *my_addr,   // socket address (generic type)
    int addrlen                 // length of socket address
);
```
Let’s have an example that binds the socket to the host the
program is running on, port 3490:
```c
struct addrinfo hints, *res;
int sockfd;

memset(&hints, 0, sizeof hints);

hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE;

getaddrinfo(NULL, "3490", &hints, &res);

sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

bind(sockfd, res->ai_addr, res->ai_addrlen);
```

And remember that, the pesky `Address already in use` error pops up because of this function call. If we want to reuse the port without getting an error, we need to use `SO_REUSEADDR` flag:
```c
int yes = 1;
if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
    perror("setsockopt");
    exit(1);
}
```

### `connect()` - Hey, you!
In simple terms, this syscall is used to connect to a remote host. It takes 3 arguments:
```c
int connect(
    int sockfd, 
    struct sockaddr *serv_addr, 
    int addrlen
);
```
I think we can predict its usage from the definition:
```c
// example of bind()

connect(sockfd, res->ai_addr, res->ai_addrlen);
```

Conclude that, `connect()` is used if you want your program should behave like a **client**.


### `listen()` - Will somebody please call me?
If you want that remote hosts should connect to you, you should use `listen()` system call:
```c
int listen(
    int sockfd,  // socket descriptor
    int backlog  // maximum number of pending connections
);
```
It simply accepts the socket descriptor and queue size to handle incoming connections synchronously.

If you want to make your program behave like a **server**, you should use `listen()`.


### `accept()` - Thank you for calling port 3490.

What’s going to happen is this: 
- someone far far away will try to `connect()` to your machine on a port that you are `listen()`ing on. Their connection will be queued up waiting to be `accept()`ed. 
- You call `accept()` and you tell it to get the pending connection. 
- It’ll return to you a *brand new socket file descriptor* to use for this single connection! 
- That’s right, suddenly you have two socket file descriptors for the price of one! The original one is still listening for more new connections, and the newly created one is finally ready to `send()` and `recv()`. We’re there!

The function prototype is:
```c
int accept(
    int sockfd,             // socket descriptor
    struct sockaddr *addr,  // generic socket address
    socklen_t *addrlen      // length of socket address
);
```


### `send()` and `recv()` - Talk to me, baby!

These two functions are for communicating over **stream sockets** or ***connected datagram* sockets**.
```c
int send(
    int sockfd,         // socket descriptor
    const void *msg,    // message to send
    int len,            // length of message
    int flags           // flags, like `MSG_NOSIGNAL`
);
int recv(
    int sockfd,         // socket descriptor
    void *buf,          // buffer to receive into
    int len,            // length of buffer
    int flags           // flags, like `MSG_WAITALL`
);
```

> ‼️ These 2 functions are `blocking`. To achieve efficiency, use multithreading or concurrency using event driven structures.

We already used these functions in this project and also in [RestaCore][github.com/Samandar-Komilov/restacore], so we don't necessarily need examples.


### `sendto()` and `recvfrom()` - Talk to me, but in DGRAM style!
These are used to communicate over **unconnected datagram sockets**. Since datagram sockets are not connected to remote host, we have to pass the destinationn host as argument:
```c
int sendto(
    int sockfd,                 // socket descriptor
    const void *msg,            // message to send
    int len,                    // length of message
    unsigned int flags,         // flags, like `MSG_NOSIGNAL`
    const struct sockaddr *to,  // destination address
    socklen_t tolen             // length of destination address
);

int recvfrom(
    int sockfd,                 // socket descriptor
    void *buf,                  // buffer to receive into
    int len,                    // length of buffer
    unsigned int flags,         // flags, like `MSG_WAITALL`
    struct sockaddr *from,      // address of sender
    int *fromlen                // length of sender address
);
```
As we can see, it is exactly the same as `send` and `recv` but we need to give destination address and its length as arguments. Why? Because this is connectionless datagram sockets. We simply throw the data and don't care it reaches the destination or not. We simply open our hands if any such packet is coming or not, and don't care whether came or not.


### `close()` - Close the door, `shutdown` - Get out of my face!
Closing a socket is as important as freeing the dynamic memory:
```c
close(int fd);

int shutdown(
    int sockfd, 
    int how
);
```
But what about `shutdown()`? 
Just in case you want a little more control over how the socket closes, this allows you to cut off communication in a certain direction, or both ways:
- 0 = Further receives are disallowed
- 1 = Further sends are disallowed
- 2 = Further sends and receives are disallowed (`close()`)


### `getpeername()` and `gethostname()` - Who are you? Sorry, who am I actually?
This function is so easy, and we can guess what it does and how:
```c
int getpeername(
    int sockfd,             // socket descriptor
    struct sockaddr *addr,  // socket address
    int *addrlen            // length of socket address
);
int gethostname(
    char *hostname,         // buffer to receive hostname
    size_t size             // length of buffer
);
```

