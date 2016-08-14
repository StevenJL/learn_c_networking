#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void exit_with_user_message(const char *msg, const char *detail) {
  fputs(msg, stderr);
  fputs(detail, stderr);
  fputs(detail, stderr);
  fputc('\n', stderr);
  exit(1);
}

void exit_with_system_message(const char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[]) {
  // Checks the number of arguments to be either 3 or 4.
  if (argc < 3 || argc > 4)
    exit_with_user_message("Parameter(s)", "<Server Address> <Echo Word> [<Server Port>]");

  // Parses ip address and echo string from arguments
  char *server_ip = argv[1];
  char *echo_string = argv[2];

  /*
   * (optional) Port for server. Defaults to 7 if not provided.
   * `in_port_t` is provided by netinet/in.h and is a type for storing ports.
   */
  in_port_t server_port = (argc == 4) ? atoi(argv[3]) : 7;

  /*
   * Create a TCP stream socket
   * AF_NET means create a socket of type IPv4
   * SOCK_STREAM means a stream-based socket
   * IPPROTO_TCP means use the TCP protocol
   */
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (sock < 0)
    exit_with_system_message("socket() failed");

  /*
   * Construct the server address structure
   * `sockaddr_in` is provided by netinet/in.h and is used to store internet addresses.
   */
  struct sockaddr_in server_address;

  // zero out server_address
  memset(&server_address, 0, sizeof(server_address));

  // IPv4 address family
  server_address.sin_family = AF_INET;

  /*
   * Convert the server's internet address (passed as a command-line argument string in
   * dot notation like xxx.xxx.xxx.xxx) into a 32-bit binary representation and set it as
   * the address portion in the `server_address` struct. We use the `inet_pton()` function
   * provided by arpa/inet.h to do this.
   */
  int conversion_result = inet_pton(AF_INET, server_ip, &server_address.sin_addr.s_addr);
  if (conversion_result == 0)
    exit_with_user_message("inet_pton() failed", "invalid address string");
  else if (conversion_result < 0)
    exit_with_system_message("inet_pton() failed");

  // Set the port portion of the `server_address` struct.
  server_address.sin_port = htons(server_port);


}

