#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <iostream>
#include <cstring>

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

void udp_chat_loop(int sock_fd, sockaddr_in peer_addr)
{
  fd_set readfds;
  char buffer[BUFF_SIZE];

  socklen_t addrlen = sizeof(peer_addr);

  while(true)
  {
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(sock_fd, &readfds);
    int max_fd = (STDIN_FILENO > sock_fd ? STDIN_FILENO : sock_fd) + 1;

    int ready = select(max_fd, &readfds, nullptr, nullptr, nullptr);
    if(ready < 0)
    {
      fail("Failed select");
    }
    if(FD_ISSET(sock_fd, &readfds))
    {
      ssize_t n = recvfrom(sock_fd, buffer, BUFF_SIZE, 0, (sockaddr*)&peer_addr, &addrlen);
      if(n <= 0)
      {
        std::cout << "Disconnected\n";
        break;
      }
      write(STDIN_FILENO, buffer, n);
    }
    if(FD_ISSET(STDIN_FILENO, &readfds))
    {
      ssize_t n = read(STDIN_FILENO, buffer, BUFF_SIZE);
      if(n <= 0)
      {
        break;
      }
      sendto(sock_fd, buffer, n, 0, (sockaddr*)&peer_addr, addrlen);
    }
  }
}

int main(int argc, char* argv[])
{
  if(argc < 4)
  {
    std::cerr<<"Usage:\n";
    std::cerr<<" TCP Server: "<< argv[0]<<" server tcp port <port>\n";
    std::cerr<<" TCP Clinet: "<< argv[0]<<" clinet tcp <host> <port>\n";
    std::cerr<<" UDP Server: "<< argv[0]<<" server udp port <port>\n";
    std::cerr<<" UDP Clinet: "<< argv[0]<<" clinet udp <host> <port>\n";
    return 1;
  }
  std::string mode = argv[1];
  std::string protocol = argv[2];
  if(protocol == "tcp")
  { 
    if(mode == "server")
    {
      int port = std::stoi(argv[3]);
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
      if(argc < 5)
      {
        std::cerr<<"Usage: "<<argv[0]<<" client <host> <port>\n";
        return 1;
      }

      const char* host = argv[3];
      int port = std::stoi(argv[4]);

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
  }
  else if(protocol == "udp")
  {
    if (mode == "server") {
            int port = std::stoi(argv[3]);
            int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock_fd < 0) fail("socket");

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = htonl(INADDR_ANY);

            if (bind(sock_fd, (sockaddr*)&addr, sizeof(addr)) < 0) fail("bind");

            sockaddr_in client_addr{};
            socklen_t addrlen = sizeof(client_addr);

            char buffer[BUFF_SIZE];
            ssize_t n = recvfrom(sock_fd, buffer, BUFF_SIZE, 0,
                                 (sockaddr*)&client_addr, &addrlen);
            if (n > 0) {
                write(STDOUT_FILENO, buffer, n);
            }

            udp_chat_loop(sock_fd, client_addr);
            close(sock_fd);

        } else if (mode == "client") {
            if (argc < 5) {
                std::cerr << "Usage: client udp <host> <port>\n";
                return 1;
            }
            const char* host = argv[3];
            int port = std::stoi(argv[4]);

            int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
            if (sock_fd < 0) fail("socket");

            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(port);
            if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) fail("inet_pton");

            const char* hello = "Hello server!\n";
            sendto(sock_fd, hello, strlen(hello), 0,
                   (sockaddr*)&server_addr, sizeof(server_addr));

            udp_chat_loop(sock_fd, server_addr);
            close(sock_fd);
        }
  }
  else
  {
  std::cout<<"Unknown protocol"<<std::endl;
  }
  return 0;
}
