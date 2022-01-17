#ifndef TCPSERVER_HPP
#define TCPSERVER_HPP

#include <cstdint>
#include <functional>
#include <list>
#include <thread>

#ifdef _WIN32 // Windows NT

#include <WinSock2.h>

#else // *nix

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#endif

#include "settings.hpp"

struct TcpServer {
  class Client;
  //Тип Callback-функции обработчика клиента
  typedef std::function<void(Client)> handler_function_t;
  //Статус сервера
  enum class status : uint8_t {
    up = 0,
    err_socket_init = 1,
    err_socket_bind = 2,
    err_socket_listening = 3,
    close = 4
  };

private:
  uint16_t m_port; //Порт
  status m_status = status::close;
  handler_function_t m_handler;

  std::thread m_threadHandler;
  std::list<std::thread> m_clientHandlerThreads;
  std::list<std::thread::id> m_clientHandlingEnd;

#ifdef _WIN32 // Windows NT
  SOCKET serv_socket = INVALID_SOCKET;
  WSAData w_data;
#else // *nix
  int m_servSocket;
#endif

  void handlingLoop();

public:
  TcpServer(const uint16_t l_port, handler_function_t l_handler);
  ~TcpServer();

  //! Set client handler
  void setHandler(handler_function_t l_handler);

  uint16_t getPort() const;
  uint16_t setPort(const uint16_t l_port);

  status getStatus() const { return m_status; }

  status restart();
  status start();
  void stop();

  void joinLoop();
};

class TcpServer::Client {
#ifdef _WIN32 // Windows NT
  SOCKET socket;
  SOCKADDR_IN address;
  char buffer[buffer_size];

public:
  Client(SOCKET socket, SOCKADDR_IN address);
#else // *nix
  int socket;
  struct sockaddr_in address;
  char m_buffer[CLIENT_INPUT_BUFFER_SIZE];

public:
  Client(int socket, struct sockaddr_in address);
#endif
public:
  Client(const Client &other);
  ~Client();
  uint32_t getHost() const;
  uint16_t getPort() const;

  int loadData();
  char *getData();

  bool sendData(const char *buffer, const size_t size) const;
};

#endif // TCPSERVER_H
