/*
  A web server, by definition, understands and processes the HTTP network
  protocol (https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol).
  Let's build a minimal web server.
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
  `int read_line(int sockfd, unsigned char *dest_buffer)` will read bytes from
  the socket `sockfd` and write these bytes to `dest_buffer` until it receives
  the EOL char.

  It returns the number of bytes written to buffer minus the EOL bytes.
*/
int read_line(int sockfd, unsigned char *dest_buffer) { 
  #define EOL "\r\n" // End-of-line byte sequence
  #define EOL_SIZE 2

  int eol_indx = 0; // index for EOL matching

  unsigned char *ptr; 
  ptr = dest_buffer;

  /*
     `recv(int socket, void *buffer, size_t length, int flags)` receives a message from the socket `socket`
     and stores the message at the location pointed to by the pointer `buffer`. The `length` specifies the
     number of chars written to the buffer. 
     
     In the invocation below, we are reading only one byte from the socket `sockfd` and storing it in the buffer that
     is pointed by `ptr`.
 */
  while(recv(sockfd, ptr, 1, 0) == 1) {
    // *ptr matches \r, the first char of the EOL sequence
    if(*ptr == EOL[eol_indx]) { 
      eol_indx++; // increment so we can next compare to \n
      if(eol_matched == EOL_SIZE) { 
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
  // Didn't find the end-of-line characters. Note buffer dest_buffer still has the message from the `sockfd`.
  return 0; 
}

/*
  `void handle_connection(int sockfd, struct sockaddr_in *client_addr_ptr)` handles the connection.

   `sockfd` is the socket file descriptor for the client with address pointed to by `client_addr_ptr`.
*/
void handle_connection(int sockfd, struct sockaddr_in *client_addr_ptr) {
  unsigned char *http_check;
  unsigned char *url;
  unsigned char request[500]; 
  unsigned char resource[500];
  int fd;
  int length;

  // copy line from `sockfd` socket and save in `request' string
  length = read_line(sockfd, request);

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

    if(strncmp(request, "GET ", 4) == 0)
      // If it's a GET request, then set pointer `url` to point at start of the request url
      url = request+4;

    if(strncmp(request, "HEAD ", 5) == 0)
      // If it's a HEAD request, then set pointer `url` to point at start of the request url
      url = request+5;

     if(url == NULL) {
       // Unknown Request
       printf("Unknown Request\n");
     } else {
       // valid request, with `url` pointer to the url 
        
     }



  }
  method = 

}


