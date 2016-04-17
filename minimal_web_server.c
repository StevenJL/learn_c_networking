/*
  A web server, by definition, understands and processes the HTTP network
  protocol (https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol).
  Let's build a minimal web server, one that only handles GET and HEAD requests.
*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>

/* 
  The HTTP protocol defaults to port 80 when not not explicitly stated otherwise.
  From RFC2616 (http://www.rfc-editor.org/rfc/rfc2616.txt):

    The http scheme is used to locate network resources via the HTTP protocol. 
    ... If the port is empty or not given, port 80 is assumed.
 */
#define PORT 80

#define WEBROOT "./mws_root"

/* 
   `int get_file_size(int fd)` returns the size of the file associated with file descriptor `fd`.
   Returns -1 on failure.
*/
int get_file_size(int fd) { 
   
  /* 
    `struct stat` is defined in sys/stat and is used by `fstat` to find the size
    of a given file among other things.
  */
  struct stat stat_struct;
  if( fstat(fd, &stat_struct) == -1) 
    return -1;
  return (int) stat_struct.st_size; 
}

/*
   `int send_string(int sockfd, unsigned char *buffer)` sends the string `buffer` to the 
   socket `sockfd`.  Returns 1 on success and 0 on failure. 
*/
int send_string(int sock_fd, char *buffer) { 
  int sent_bytes;
  int bytes_to_send;
  bytes_to_send = strlen(buffer); 
  while(bytes_to_send > 0) {
    sent_bytes = send(sock_fd, buffer, bytes_to_send, 0); 
    if(sent_bytes == -1)
      return 0; // Return 0 on send error.
    bytes_to_send -= sent_bytes;
    buffer += sent_bytes; 
  }
  return 1; // Return 1 on success. 
}

/*
  `int read_line(int sockfd, unsigned char *dest_buffer)` will read bytes from
  the socket `sockfd` and write these bytes to `dest_buffer` until it receives
  the EOL char.

  It returns the number of bytes written to buffer minus the EOL bytes.
*/
int read_line(int sock_fd, char *dest_buffer) { 
  #define EOL "\r\n" // End-of-line byte sequence
  #define EOL_SIZE 2

  int eol_indx = 0; // index for EOL matching

  char *ptr; 
  ptr = dest_buffer;

  /*
     `recv(int socket, void *buffer, size_t length, int flags)` receives a message from the socket `socket`
     and stores the message at the location pointed to by the pointer `buffer`. The `length` specifies the
     number of chars written to the buffer. 
     
     In the invocation below, we are reading only one byte from the socket `sock_fd` and storing it in the buffer that
     is pointed by `ptr`.
 */
  while(recv(sock_fd, ptr, 1, 0) == 1) {
    // *ptr matches \r, the first char of the EOL sequence
    if (*ptr == EOL[eol_indx]) { 
      eol_indx++; // increment so we can next compare to \n
      if (eol_indx == EOL_SIZE) { 
        // We found \r\n, the EOL byte sequence.
        *(ptr+1-EOL_SIZE) = '\0'; // terminate the string.

        // We've received all the data up to the EOL, so stop writing and return the number of chars written to the buffer.
        return strlen(dest_buffer);
      }
    } else { 
      eol_indx = 0;
    }
    ptr++;
  }
  // Didn't find the end-of-line characters. Note buffer dest_buffer still has the message from the `sock_fd`.
  return 0; 
}

/*
  `void process_request(int sockfd, struct sockaddr_in *client_addr_ptr)` processes the incoming http request.

   `sockfd` is the socket file descriptor for the client with address pointed to by `client_addr_ptr`.
*/
void process_request(int client_sock_fd, struct sockaddr_in *client_addr_ptr) {
  char *http_check;
  char *url;
  char *file;
  char request[500]; 
  char resource[500];
  int resource_fd;
  int length;
  int file_size;

  // copy line from `client_sock_fd` socket and save in `request' string
  length = read_line(client_sock_fd, request);

  // log client address, port and request
  printf(
    "Client Address: %s\nClient Port: %d\nRequest: %s\n",
    inet_ntoa(client_addr_ptr->sin_addr),
    ntohs(client_addr_ptr->sin_port),
    request
  );

  /*
    If the request is a valid http request, it would be a string that looks something like:

      GET /path/my/awesome/webpage.html HTTP/1.0

      We verify its a valid HTTP request by checking for the substring "HTTP"
      Note that http_check is a pointer to the first char of the substring.
  */
  http_check = strstr(request, " HTTP/");

  if (http_check == NULL){
    // Not valid HTTP request
    printf(" Not valid HTTP Request.\n");
  } else {
    /* 
       We first remove the HTTP part from string `request` (since we already know its an HTTP request)
       by setting the `http_check` pointer (which is currently pointing to the space before H) to 0.
    */
    *http_check = 0; 

    /* 
       Let's parse the url, but first set it to the NULL pointer so we know if we successfully
       parsed it.
    */ 
    url = NULL;

    if (strncmp(request, "GET ", 4) == 0)
      // If it's a GET request, then set pointer `url` to point at start of the request url
      url = request+4;

    if (strncmp(request, "HEAD ", 5) == 0)
      // If it's a HEAD request, then set pointer `url` to point at start of the request url
      url = request+5;

     if (url == NULL) {
       // Unknown Request
       printf("Unknown Request\n");
     } else {

       // If the url string ends in '/', just add on index.html at the end.
       if (url[strlen(url) -1] == '/')
         strcat(url, "index.html");

       strcpy(resource, WEBROOT);
       strcat(resource, url);
        
       // Connect to the file in read-only mode
       resource_fd = open(resource, O_RDONLY, 0);

       printf("Resource Requested: %s \n", resource);

       if (resource_fd == -1) { 
         // If file is not found
         printf("404 Not Found\n");
         send_string(client_sock_fd, "HTTP/1.0 404 NOT FOUND\r\n");
         send_string(client_sock_fd, "Server: Minimal Web Server\r\n\r\n"); 
         send_string(client_sock_fd, "<html><head><title>404 Not Found</title></head>"); 
         send_string(client_sock_fd, "<body><h1>URL not found</h1></body></html>\r\n");
       } else {
         // File is found
         
         // If it's a GET request
         if (strncmp(request, "GET ", 4) == 0) {
           // serve up the header and the requested file
           send_string(client_sock_fd, "HTTP/1.0 200 OK\r\n");
           send_string(client_sock_fd, "Server: Tiny webserver\r\n\r\n");

           // Determine the file size in bytes
           file_size = get_file_size(resource_fd);

           // Allocate memory to store the file in the local variable `file`.
           file = (char *) malloc(length);

           // Read target file into memory and save in local `file`. 
           read(resource_fd, file, file_size);

           // Send file to client 
           send(client_sock_fd, file, file_size, 0);

           // free memory for file
           free(file);
         }

         // If it's a HEAD request
         if (strncmp(request, "HEAD ", 5) == 0) {
           // just serve up the header
           send_string(client_sock_fd, "HTTP/1.0 200 OK\r\n");
           send_string(client_sock_fd, "Server: Tiny webserver\r\n\r\n");
         }

         // close the file
         close(resource_fd);
       }
     }
  }

  /*
     `int shutdown(int socket, int how)` gracefully shutdowns socket send and receive operations.
     Namely, it shutdowns the socket `socket` according to the `how` argument.  Parameters for `how` include:

     1) `SHUT_RD` to shutdown receive operations.
     2) `SHUT_WR` to shutdown send operations.
     3) `SHUT_RDWR` to shutdown send and receive operations.
  */
  shutdown(client_sock_fd, SHUT_RDWR);
}

int main(void) {
  int host_sock_fd;
  int client_sock_fd;
  int option_value = 1;

  struct sockaddr_in host_addr;
  struct sockaddr_in client_addr;
  socklen_t sin_size;

  printf("Starting Minimal Web Server on Port %d\n", PORT);

  /* 
    `int socket(int domain, int type, int protocol) creates a socket. It returns the file descriptor
    for the socket (as an int). Recall that in Unix everything is a file including sockets. It returns 
    -1 if there was an error while creating. 
    
    The `PF_INET` argument is defined in /usr/include/bits/socket.h and is equal to the integer 2.  It tells 
    `socket()` that we want to create a socket for the IP protocol family. 

    The `SOCK_STREAM` argument is defined in /usr/include/bits/socket.h and is equal to the integer 1. It tells 
    `socket()` that we want to create a socket that uses TCP (as opposed to UDP). 
    
    The 3rd argument of 0 is used to differentiate protocols if there are multiple protocols within a family.
    In this case, the PF_INET family has only one protocol, so we use 0.
  */
  if ((host_sock_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    printf("Failed to create socket");
    return 1;
  }

  /*
    `int setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len)`
    configures the socket options by modifying it to the value pointed to by `option_value`.

    The first argument, `socket`, is the socket we want to create a configuration for.  The second argument,
    `level` also describes a configuration, a mysterious "level" apparently. I just know its also a 
    part of the socket configuration. The offical documentation on level states: 

      "The level argument specifies the protocol level at which the option resides. To set options at 
      the socket level, specify the level argument as SOL_SOCKET."

    The third argument is a pointer, `option_value`.  This is the value that actually configures the socket.
     
    In our invocation below, the `level` argument is assigned to SOL_SOCKET which is defined in sys/socket.h 
    and is equal to the hexidecimcal 0xffff.  We do this because we want to configure it at the "socket level".

    In our invocation below, the `option_name` argument is assigned to `SO_REUSEADDR`, and the configuration
    value for this option is a pointer to `option_value`, which we initialized as 1 (true) earlier. This 
    will configure this socket so that we can re-use a socket for binding to a given address. For example, 
    if a previous used socket was not closed properly, it would appear to be in use. This option lets us use 
    that socket anyway.
  */
  if (setsockopt(host_sock_fd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(int)) == -1) {
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
   You may notice below in the setting of the ip address of the host, it's set to zero.  That doesn't
   mean that the host internet address is 0 (which doesn't make sense, since an ip address
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
    This is where the magic happens.  By binding `host_sock_fd` to the host address, we can now
    read from the host address/port like a file.  Note `host_sock_fd` must not already be
    bound to an address for this invocation to work.

    Note also, we are converting host_addr from type struct sockaddr_in to type struct sockaddr.
    The reason for this is because `bind` accepts a `sockaddr` type, not a `sockaddr_in`. So why
    were we using `sockaddr_in` in the first place? It's because it's the struct to use for the 
    internet ("sockaddr_in" is short for "socket address internet") and it's easier to work with. 
    This trick of using sockaddr_in and then typecasting it to sockaddr is a common networking hack.
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

    In the invocation below, the suggested size of the listen queue is set to 20.
  */
  if (listen(host_sock_fd, 20) == -1) {
    printf("%s", "Could not listen to socket\n");
    return 1;
  }

  while(1) { // basically run this process forever until control-C'ed
    sin_size = sizeof(struct sockaddr_in); // get the size of struct type sockaddr_in

    /*
      `int accept(int socket, struct sockaddr address, socklen_t address_len)` accepts a new connection on `socket`.
      Technically, it accepts the first connection on the queue of pending connections, creates a new socket
      with the same configuration as `socket` and allocates a new file descriptor for that new socket
      and then returns that file descriptor, which is connected to a client. 
      
      Note this is a blocking call. ie. this will hang until a connection comes in.

      Referring to the invocation below, `client_sock_fd` is connected to a remote client and cannot accept more
      connections.  However `host_sock_fd` remains open and can accept new connections.

      `accept` also stores the connections address info in `struct sockaddr address`. Referring to the
      invocation below, the client's address information gets stored in `client_addr` struct,
      but note we typecast it to the sockaddr type. Again, note the typcasting from sockaddr_in to sockaddr
      trick in use again here.
    */
    client_sock_fd = accept(host_sock_fd, (struct sockaddr *)&client_addr, &sin_size); 

    if (client_sock_fd == -1) {
      printf("%s", "Socket failed to accept.");
      return 1;
    }

    process_request(client_sock_fd, &client_addr);
  }

  return 0;
}

