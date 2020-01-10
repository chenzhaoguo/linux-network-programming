# linux-network-programming


## 主机字节序与网络字节序

对于跨越多个字节的程序对象，我们必须建立两个规则：这个对象的地址是什么？以及在内存中如何排列这些字节。在几乎所有的机器上，多字节对象都被存储为连续的字节序列，对象的地址为所使用的字节中最小的地址。


计算机中一个整数的字节序分为大端字节序(big endian)与小端字节序(little endian)。大端字节序是指一个整数的高位字节存（23~31 bit）储在内存的低地址处，低位字节（0~7 bit）存储在内存的高地址处。而小端法则刚好相反。

现代PC大多采用小端字节序，因此小端字节序被称为主机字节序。而一般两台计算之间通信，我们必须约束好彼此双方发送信息时使用的字节序，一般约定为使用大端字节序，所以我们一般把大端字节序称为网络字节序。

大端法符合人们的读写顺序，而小端法则适合计算机的计算，计算机的电路先处理低字节会比较高效。

> 大端与小端源于对鸡蛋该从哪一端打开的争论

需要注意的大小端的问题只针对计算该如何解析内存中的整数，而对于浮点数、字节流等数据是不存的。操作系统提供了4个函数（`htonl`/`ntohl`/`htons`/`ntohs`），分别用于`uint16/unsigned short`以及`uint32/unsigned long`类型的整数在主机字节序与网络字节序之间的转换。这4个函数存在头文件`netinet/in.h`中。

主机大小端判断以及主机与网络序转换的示例程序：[socket-api/big_little_endian.c](socket-api/big_little_endian.c)

## Socket API

Linux的网络API可以分类3类：

- socket地址API，主要描述通过什么样的数据结构来描述一个socket的地址`(ip, port)`
- socket基础API，包括了socket创建、关闭、命名、监听、接受连接、发起连接、发送与接收数据以及读取和设置socket的选项。
- 网络信息API，它提供了ip地址和域名、端口号和服务名之间的转换

### Socket地址

### Socket地址的结构

目前常见的协议族的定义如下：

协议族 | 地址族 | 描述 | 地址值的含义
:---: | :---: | :---: | :---:
`PF_UNIX` | `AF_UNIX`| `UNIX`本地域协议族 | 文件的路径名，长度可达到108个字节
`PF_INET` | `AF_INET` | `TCP/IPv4`协议族 | 2字节的端口号和4字节的IPv4地址，共6字节
`PF_INET6` | `AF_INET6` | `TCP/IPv6`协议簇 | 2字节的端口号，4字节流标识，16字节的IPv6地址，4字节的范围ID，一共26字节。

宏`PF_*`和`AF_*`都在`bits/socket.h`头文件中，且后者与前者有完全相同的值，所以二者通常混用。

所有的socket的API中所使用的地址的结构都是通用的地址结构，即`sockaddr`，但由于`sockaddr`的结构并不能满足不同协议族的需求，所以各个协议族又定义了自己的专用地址结构，它们的关系如下图所示：

![](./assets/sock_addr.png)

由于我们在socket API中只能使用`sockaddr`的结构，所以所有专用的地址结构，必须通过强制类型转换来转换为通用类型。

### 点分址进制ip字符串与网络序的IP地址之间的转换

对于ip来地，人们习惯用点分十进制来表示IPv4的地址，用十六进制字符串来表示IPv6的地址，但是在编程里，我们需要转化为整数（二进制数），
当我们需要写日志的时候，我们又需要以可读的形式来记录。

Linux Socket API提供了两组函数来提供这种转换功能，其中`inet_addr/inet_aton/inet_ntoa`应试是比较早的接口，不支持IPv4，同时还有不可重入的问题。后续应该会被弃用，新的一组接口功能强大一些。

![](./assets/ip_translation.png)

示例程序：[socket-api/ip_translation.c](socket-api/ip_translation.c)

## Socket基础API

### socket: 创建Socket

```c
#include <sys/types.h>
#include <sys/socket.h>

///@brief 创建socket句柄
///@param[in] domain
///用于告诉系统使用哪个底层协议族，可选的一般有：PF_INET、PF_INET6、PF_UNIX
///@param[in] type
/// 用于指定服务类型。主要类型有SOCK_STREAM，用于说明是流服务，也就是TCP连接。SOCK_DGRAM表示数据报服务，也就是UDP连接。自Linux
/// 2.6.17起，type参数可以接受上述服务类型与下面两个重要的标志相与的值。SOCK_NONBLOCK表示将新创建的socket设为非阻塞的；
/// SOCK_CLOEXEC表示用fork调用创建子进程中关闭该
/// socket。在Linux 2.6.17之前，这两个属性都需要使用额外的系统调用fcntl来设置。
///@param[in] protocol
/// 参数是在前两个参数构成的协议集合下，再选择一个具体的协议。
/// 不过这个值通过是唯一的（前两个参数已经完全全决定了它的值。）几乎在所有情况下，我们都应该把它设置为0，表示使用默认协议。
///@return
///系统成功，将返回一个文件描述系，它像其他文件描述符一样，是可读、可写、可控制、可关闭的。失败就返回-1，并设置errno。
int socket(int domain, int type, int protocol);
```
### bind: Socket绑定地址

当我们调用`socket()`函数创建了一个Socket后，虽然指定了地址族，但并未指定使用这个地址族中哪个具体的地址。

在服务器端程序中，我们需要指定一个具体的地址，这样与它通信的客户端才能通过这个地址来进行连接与通信。而在客户端程序中就不需要，因为操作系统会自动的分配一个socket地址（主要是端口号，地址一般就是客户端机器的IP地址）。


```cpp
#include <sys/types.h>
#include <sys/socket.h>

/// @brief 给一个socket文件描述符绑定一个具体的socket地址
/// @param[in] sockfd　socket的文件描述符
/// @param[in] my_addr socket的地址，这里的类型虽然为`socketaddr*`，
/// 但我们可以用专用地址(`sockaddr_in`/`sockaddr_in6`)来构建，然后再强制转换
/// @param[in] addrlen socket地址的长度
/// @retrun bind成功时返回0，失败时返回-1并设置`errno`。其中两种常见的`errno`是`EACCESS`和`EADDRINUSE`
/// `EACCESS`表示被绑定的地址是受保护的地址，仅超级用户能够访问。比如普通用户将socket绑定到知名的服务端口(0~1023)
/// `EADDRINUSE`表示被绑定的地址正在使用中。比如将socket绑定到一个处于`TIME_WAIT`状态的socket地址
int bind(int sockfd, const struct sockaddr* my_addr, socklen_t addrlen);
```

### listen: 监听socket

```c
/// @brief 给socket创建一个监听队列以存放待处理的客户连接
/// @param[in] sockfd　socket的文件描述符
/// @param[in] backlog　参数提示内核监听队列的最大长度。
/// 监听队列的长度如果超过backlog，服务器将不再受理新的客户连接，
/// 客户端也将收到`ECONNREFUSED`的错误信息。
/// 在内核版本2.2前，backlog的参数是指所有处理半连接和完全连接状态的socket的上限。
/// 内核版本2.2后，它只表示处于完全连接状态的socket的上限
/// 处于半连接状态的socket的上限则由`/proc/sys/net/ipv4/tcp_max_syn_backlog`内核参数定义
/// backlog的典型值是5
#include <sys/socket.h>
int listen(int sockfd, int backlog);
```

对于`backlog`的理解：当服务器调用`listen`后，就一直等待客户端来建立连接，这里如果客户端调用了`connect`，那么服务器端就会一连接建立的过程，这个过程会先经过`SYN_RCVD`再到`ESTABLISHED`。这些半连接和完全连接状态的连接保存在两个不同的队列中。
这时如果服务器端一直没有调用`accept`来从`ESTABLISHED`队列中取走连接来处理，那队列满后，就不能再响应连接建立了。