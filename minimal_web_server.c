/*
  A web server, by definition, understands and processes the HTTP network
  protocol (https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol).
  Let's build a minimal web server, one that only response to GET and HEAD requests.
*/

#include <stdio.h>
#include <fcntl.h>
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

#define WEBROOT "./webroot"

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
int send_string(int sock_fd, unsigned char *buffer) { 
  int sent_bytes, 
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
int read_line(int sock_fd, unsigned char *dest_buffer) { 
  #define EOL "\r\n" // End-of-line byte sequence
  #define EOL_SIZE 2

  int eol_indx = 0; // index for EOL matching

  unsigned char *ptr; 
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
      if (eol_matched == EOL_SIZE) { 
        // We found \r\n, the EOL byte sequence.
        *(ptr+1-EOL_SIZE) = '\0'; // terminate the string.

        // We've received all the data up to the EOL, so stop writing and return the number of chars written to the buffer.
        return strlen(dest_buffer);
      }
    } else { 
      eol_matched = 0;
    }
    ptr++;
  }
  // Didn't find the end-of-line characters. Note buffer dest_buffer still has the message from the `sock_fd`.
  return 0; 
}

/*
  `void handle_connection(int sockfd, struct sockaddr_in *client_addr_ptr)` handles the connection.

   `sockfd` is the socket file descriptor for the client with address pointed to by `client_addr_ptr`.
*/
void handle_connection(int client_sock_fd, struct sockaddr_in *client_addr_ptr) {
  unsigned char *http_check;
  unsigned char *url;
  unsigned char request[500]; 
  unsigned char resource[500];
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

       printf("Resource Requested: %s \n", resource)

       if (resource_fd == -1) { 
         // If file is not found
         print("404 Not Found\n");
         send_string(client_sock_fd, "HTTP/1.0 404 NOT FOUND\r\n");
         send_string(client_sock_fd, "Server: Minimal Web Server\r\n\r\n"); 
         send_string(client_sock_fd, "<html><head><title>404 Not Found</title></head>"); 
         send_string(client_sock_fd, "<body><h1>URL not found</h1></body></html>\r\n");
       } else {
         // File is found
         if (strncmp(request, "GET ", 4) == 0) {
           // If it's a GET request serve up the header and the requested file
           send_string(client_sock_fd, "HTTP/1.0 200 OK\r\n");
           send_string(client_sock_fd, "Server: Tiny webserver\r\n\r\n");
           file_size = get_file_size(resource_fd);
         }
         if (strncmp(request, "HEAD ", 5) == 0) {
           // If it's a HEAD request serve up just the header
         }
       }
     }
  }
}

