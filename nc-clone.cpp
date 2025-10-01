#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <iostream>

const size_t BUFF_SIZE = 1024;

void fail(const char* str)
{
  perror(str);
  exit(1);
}

void chat_loop(int sock_fd)
{
  fd_set readfds;
  char buffer[BUFF_SIZE];

  while(true)
  {
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(sock_fd, &readfds);
    int max_fd = (STDIN_FILENO >sock_fd ? STDIN_FILENO : sock_fd)+1;

    int ready = select(max_fd, &readfds, nullptr, nullptr, nullptr);
    if(ready < 0)
    {
      fail("Select failed");
    }

    if(FD_ISSET(sock_fd, &readfds))
    {
      ssize_t n = read(sock_fd, buffer, BUFF_SIZE);
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
      write(sock_fd, buffer, n);
    }
  }

}

int main(int argc, char* argv[])
{
  if(argc < 3)
  {
    std::cerr<<"Usage:\n";
    std::cerr<<" Server: "<< argv[0]<<" server port <port>\n";
    std::cerr<<" Clinet: "<< argv[0]<<" clinet <host> <port>\n";
    return 1;
  }
  std::string mode = argv[1];
  if(mode == "server")
  {
    int port = std::stoi(argv[2]);
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd < 0)
    {
      fail("Failed to create the socket");
    }
    int opt = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in my_address;
    bzero(&my_address, sizeof(my_address));
    my_address.sin_family = AF_INET;
    my_address.sin_port = htons(port);
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
    chat_loop(client_fd);

    close(client_fd);
    close(sock_fd);
  }
  else if(mode == "client")
  {
    if(argc < 4)
    {
      std::cerr<<"Usage: "<<argv[0]<<" client <host> <port>\n";
      return 1;
    }

    const char* host = argv[2];
    int port = std::stoi(argv[3]);

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd < 0)
    {
      fail("Fail to create the socket");
    }
    sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port= htons(port);
    if(inet_pton(AF_INET, host, &addr.sin_addr)<=0)
    {
      fail("Failed to bind the socket");
    }
    if(connect(sock_fd, (sockaddr*)&addr, sizeof(addr))<0)
    {
      fail("Failed to connect the socket");
    }
    chat_loop(sock_fd);
    close(sock_fd);
  }
  else
  {
    std::cerr<<"Unknown mode: "<<mode<<"\n";
    return 1;
  }
  return 0;
}
