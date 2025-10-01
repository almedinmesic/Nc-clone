#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <iostream>

const size_t BUFF_SIZE = 30;

void fail(const char* str)
{
  perror(str);
  exit(1);
}

int main()
{
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(sock_fd < 0)
  {
    fail("Failed to create the socket");
  }
  sockaddr_in my_address;
  bzero(&my_address, sizeof(my_address));
  my_address.sin_family = AF_INET;
  my_address.sin_port = htons(5000);
  my_address.sin_addr.s_addr = htonl(INADDR_ANY);
  if(bind(sock_fd, reinterpret_cast<sockaddr*>(&my_address), sizeof(my_address)) < 0)
  {
    fail("Failed to bind the socket");
  }
  if(listen(sock_fd, 5) < 0)
  {
    fail("Listen failed");
  }
  int client_fd = accept(sock_fd, nullptr, nullptr);
  if(client_fd < 0)
  {
    fail("Accept failed");
  }
  fd_set readfds;
  char buffer[BUFF_SIZE];

  while(true)
  {
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(client_fd, &readfds);
    int max_fd = (STDIN_FILENO >client_fd ? STDIN_FILENO : client_fd)+1;

    int ready = select(max_fd, &readfds, nullptr, nullptr, nullptr);
    if(ready < 0)
    {
      fail("Select failed");
    }

    if(FD_ISSET(client_fd, &readfds))
    {
      ssize_t n = read(client_fd, buffer, BUFF_SIZE);
      if(n <= 0)
      {
        std::cout<<"Clinet disconnected"<<std::endl;
        break;
      }
      write(STDOUT_FILENO, buffer, n);
    }

    if(FD_ISSET(STDIN_FILENO, &readfds))
    {
      ssize_t n = read(STDIN_FILENO, buffer, BUFF_SIZE);
      if(n <= 0)
      {
        std::cout<<"EOF, closing connection"<<std::endl;
        break;
      }
      write(client_fd, buffer, n);
    }
  }
  close(client_fd);
  close(sock_fd);
  return 0;
}
