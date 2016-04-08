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
    `socket()` returns an int referring to a socket file descriptor. Recall that in Unix everything is a file
    including networking sockets. It returns -1 if there was an error while creating. 
    
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
    `setsockopt()` sets socket options. 
    
    The first argument `host_sock_fd` is the socket we created above. 

    The second argument SOL_SOCKET specifies the protocol level at which the option resides. SOL_SOCKET
    is defined in sys/socket.h and is equal to the hexidecimcal 0xffff. For more information see this:
    http://stackoverflow.com/questions/21515946/sol-socket-in-getsockopt

    The third argument SO_REUSEADDR means we want to reuse a socket at a given address for binding.
    For example, if a previous used socket was not closed properly, it would appear to be in use.
    This option lets us use that socket anyway. We are setting the value of SO_REUSEADDR to the value
    pointed to by the fourth argument, which is equal to 1, allowing us to re-use sockets that
    appear in use. The last argumnent is the size of the third argument, which is an int. The fact
    that we are passing a pointer to `option_value` as well as its size indicates that `setsocketopt`
    may sometimes change the value of option_value.

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

    In the code below, we set `host_addr` to persist address information about the host.
  */

  host_addr.sin_family = AF_INET; // This is specifying the address type as ipv4

  /*
    `htons` which stands for host-to-network-sort is defined /usr/include/netinet/in.h
    and it converts a 16-bit integer from the host's byte order to network byte order.
    This is necessary because the network bye ordering is big-endian while the x86 processor
    encodes as little-endian.

    Anyways, we are setting the port portion of the host internet address.
  */

  host_addr.sin_port = htons(PORT);

  /*
   This sets the ip address portion of the host. You'll notice its zero.  That doesn't
   mean that the host internet address is 0 (which doesn't even make sense an ip address
   is an ordered set of 4 integers). This means automatically fill it ith my ip.
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
    `int listen(int socket, int backlog)` tells a socket to start listening.
    Technically, it starts a connection (a semi-permanent channel of communication).
    The backlog argument is used to limit the number of outstanding connections in
    the socket's listen queue.
  */
  if (listen(host_sock_fd, 5) == -1) {
    printf("%s", "Could not listen to socket\n");
    return 1;
  }

  while(1) { // basically run this process forever unless control-C'ed
    sin_size = sizeof(struct sockaddr_in); // get the size of struct type sockaddr_in

    /*
      `int accept(int socket, struct sockaddr address, socklen_t address_len)` accepts a new connection on `socket`.
      Technically, it  the first connection on the queue of pending connections, creates a new socket
      with the same configuration as `socket` and allocates a new file descriptor for that new socket
      and then returns that file descriptor.
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

    // print a message on the server showing the client's ip address and port.
    printf(
      "server: got connection from %s port %d\n", 
      inet_ntoa(client_addr.sin_addr), 
      ntohs(client_addr.sin_port)
    ); 
    
    strcpy(buffer, " Listen to John Coltrane... ");

    strcat(buffer, "By the way, your ip address is: ");
    strcat(buffer, inet_ntoa(client_addr.sin_addr));
    strcat(buffer, " \n");

    /*
      Filter out all the weird characters.  Not sure why they're in buffer
    */
    int indx;
    // I hate magic numbers as much as the next guy, but this isn't production code so w/e. 
    // The 75 you see below is the number of chars in the "List to John Coltrane... 
    // you ip address" message above.
    int message_length = 75 + sizeof(inet_ntoa(client_addr.sin_addr));
    for(indx = 0; indx < message_length; indx++){
      out_message[indx] = buffer[indx];
    }

    /*
       Sends a message to a socket.

      `ssize_t send(int socket, const void *buffer, size_t length, int flags)` will send
      the message stored in `buffer` to `socket`.  The size of the message is saved in
      `length`.  The last argument of 0 is a flag that indicates the type message
      transmission.

      Returns -1 if failed to send message.
    */
    send(client_sock_fd, out_message, sizeof(buffer), 0);
    close(client_sock_fd);
  }

  return 0; 
}


