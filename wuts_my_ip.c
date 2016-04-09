/*
  A simple server which accepts client web requests and sends back the client's ip address.
*/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9666

int main(void) {
  int host_sock_fd; // the socket that for the host
  int option_value = 1;
  struct sockaddr_in host_addr; 
  int client_sock_fd;
  socklen_t sin_size;
  struct sockaddr_in client_addr; 
  char buffer[128];
  char out_message[128];
  int recv_length = 1;

  /* 
    `int socket(int domain, int type, int protocol) creates a socket. It returns the file descriptor
    for the socket (as an int). Recall that in Unix everything is a file including sockets. It returns 
    -1 if there was an error while creating. 
    
    The `PF_INET` argument is defined in /usr/include/bits/socket.h and is equal to the integer 2.  It tells 
    `socket()` that we want to create a socket for the IP protocol family. 

    The `SOCK_STREAM` argument is defined in /usr/includebits/socket.h and is equal to the integer 1. It tells 
    `socket()` that we want to create a socket that uses TCP (as opposed to UDP). 
    
    The 3rd argument of 0 is used to differentiate protocols if there are multiple protocols within a family.
    In this case, the PF_INET family has only one protocol, so we use 0.
  */
  if ((host_sock_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    printf("%s", "Error while creating socket\n");
    return 1;
  }

  /*
    `int setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len)`
    configures the socket options by modifying the value pointed to by `option_value`.

    The first argument, `socket`, is the socket we want to create a configuration for.  The third argument,
    `option_name` describes a configuration we want to save to `option_value`. The second argument,
    `level` also describes a configuration, a mysterious "level" apparently. I just know its also
    part of the configuration of `option_value`.
     
    In our invocation below, the `level` argument is assigned to SOL_SOCKET which is defined in sys/socket.h 
    and is equal to the hexidecimcal 0xffff. 

    In our invocation below, the `option_name` argument is assigned to `SO_REUSEADDR`, which is a
    configuration we want so that we can resuse a socket for binding to a given address. For example, 
    if a previous used socket was not closed properly, it would appear to be in use.
    This option lets us use that socket anyway.
  */

  if (setsockopt(host_sock_fd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(int)) == -1 ){
    printf("%s", "Error while configuring socket's re-use option equal to true\n");
    return 1;
  }

  /*
    `host_addr` (defined earlier) is a variable of the sockaddr_in struct type, which is defined in /usr/include/netinet/in.h
    It's a structure that essentially defines an Internet Address.  This structure has four components:

    1) The address family (ipv4, ipv6, internet radio, etc.)
    2) The port number
    3) The internet address
    4) The zero padded portion

     Let's configure `host_addr` to describe the address of the host.
  */

  host_addr.sin_family = AF_INET; // This is specifying the address type as ipv4

  /*
    `htons` which stands for host-to-network-sort is defined /usr/include/netinet/in.h
    and it converts a 16-bit integer from the host's byte order to network byte order.
    This is necessary because the network byte ordering is big-endian while the x86 processor
    encodes as little-endian.

    Anyways, we are setting the port portion of the host internet address.
  */

  host_addr.sin_port = htons(PORT);

  /*
   When setting the ip address of the host, you'll notice its zero.  That doesn't
   mean that the host internet address is 0 (which doesn't even make sense an ip address
   is an ordered set of 4 integers). This means automatically fill it with the host's ip address.
  */
  host_addr.sin_addr.s_addr = 0;

  /*
    Recall there was a zero-padded portion of an internet address.  We are making that here.

    `memset(string *source, int char_to_copy, int n)` copies the `char_to_copy`  to the first
    n characters of the string pointed to by source. In this case, we are the null character
    8 times to `sin_zero` char array of the `host_addr` structure.

  */
  memset(&(host_addr.sin_zero), '\0', 8);

  /*
    This is where the magic happens.  By binding the socket to the host address, we can now
    read and write to this address as if it was a file.

    Note also, we are converting host_addr from type struct sockaddr_in to type struct sockaddr.
    Subtle.
  */
  if (bind(host_sock_fd, (struct sockaddr *)&host_addr, sizeof(struct sockaddr)) == 1) {
    printf("%s", "Failed to Bind Socket to Host Address\n");
    return 1;
  }

  /*
    `int listen(int socket, int backlog)` tells the port that's bound to this socket to 
    start listening for socket connections as they come in. Connections that come in are then
    placed in a listen queue. The backlog argument is used to limit the number of outstanding 
    connections in the socket's listen queue. I get the sense that `backlog` doesn't guarantee 
    that's the limit on the listen queue, as it's only advisory: 
    http://stackoverflow.com/questions/5111040/listen-ignores-the-backlog-argument

    In the invocation below, the suggested size of the listen queue is set to 5.
  */
  if (listen(host_sock_fd, 5) == -1) {
    printf("%s", "Could not listen to socket\n");
    return 1;
  }

  while(1) { // basically run this process forever unless control-C'ed
    sin_size = sizeof(struct sockaddr_in); // get the size of struct type sockaddr_in

    /*
      `int accept(int socket, struct sockaddr address, socklen_t address_len)` accepts a new connection on `socket`.
      Technically, it accepts the first connection on the queue of pending connections, creates a new socket
      with the same configuration as `socket` and allocates a new file descriptor for that new socket
      and then returns that file descriptor, which is now has a connection.

      Referring to the invocation below, `client_sock_fd` is connected to a remote client and cannot accept more
      connections.  However `host_sock_fd` remains open and can accept new connections.

      `accept` also stores the connections address info in `struct sockaddr address`. Referring to the
      invocation below, the client's address information gets stored in `client_addr` struct,
      but note we typecast it to the sockaddr type.
    */
    client_sock_fd = accept(host_sock_fd, (struct sockaddr *)&client_addr, &sin_size); 

    if (client_sock_fd == -1) {
      printf("%s", "Socket failed to accept.");
      return 1;
    }
     
    /*
       `client_addr` is of type struct sockaddr_in. As such, it has a member `sin_addr`.
       GOTCHA ALERT: What type is `sin_addr`? Its of type struct in_addr but note that the 
       implementation of in_addr varies from system to system! Dub Tee Eff internet standards. Tsk Tsk.
       http://stackoverflow.com/questions/13979150/why-is-sin-addr-inside-the-structure-in-addr

       `char *inet_ntoa(strut in_addr in)` is used to convert internet host address to a string
       with internet standard dot notation (xxx.xx.xxx.xxx). This method is defined in
       /usr/

       Similarly, `inet_ntohs` converts the client address port, client_addr.sin_port, from
       network byte order to host byte order. This is that big-endian vs little-endian issue
       mentioned earlier.
    */ 

    /* 
       Now that an accepted connection is represented by the `client_sock_fd` file descriptor, 
       we are going to find the ip address and port of the client. We will print a message on 
       the host showing the client's ip address and port.
    */
    printf(
      "server: got connection from %s port %d\n", 
      inet_ntoa(client_addr.sin_addr), 
      ntohs(client_addr.sin_port)
    ); 
    

    /* 
      Let's prepare a message to respond to the client and save it in the
      buffer char array.
    */
    strcpy(buffer, " Listen to John Coltrane... ");

    strcat(buffer, "By the way, your ip address is: ");
    strcat(buffer, inet_ntoa(client_addr.sin_addr));
    strcat(buffer, " \n");

    /*
      Note that `buffer` is an array of 120 chars and we only filled the first portion of 
      it with the "Listen to John Coltrane.... you ip address is" message.  The rest of the
      array is garbage memory so we don't want to print that. So we need to calculate
      a `message_length` and pass that `send` function below so we don't print garbage
      chars on the client.

      I hate magic numbers as much as the next guy, but this isn't production code so w/e. 
      The 75 you see below is the number of chars in the "List to John Coltrane... 
      you ip address" message above.
    */
    int message_length = 75 + sizeof(inet_ntoa(client_addr.sin_addr));

    /*
       Sends a message to a socket.

      `ssize_t send(int socket, const void *buffer, size_t length, int flags)` will send
      the message stored in `buffer` to `socket`.  The size of the message is saved in
      `length`.  The last argument of 0 is a flag that indicates the type message
      transmission.

      Returns -1 if failed to send message.
    */
    send(client_sock_fd, buffer, message_length, 0);

    // close the connection with the socket
    close(client_sock_fd);
  }

  return 0; 
}

