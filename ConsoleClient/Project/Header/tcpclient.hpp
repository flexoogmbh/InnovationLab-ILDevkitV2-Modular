#ifndef TCPCLIENT_HPP
#define TCPCLIENT_HPP

#include "settings.hpp"

#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <unistd.h>

class TcpClient {
public:
  TcpClient();
  ~TcpClient();
  int scanNetwork();

private:
  sockaddr_in m_serverAddress;
};

#endif // TCPCLIENT_HPP
