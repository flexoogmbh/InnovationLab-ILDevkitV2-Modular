#include "tcpServer.hpp"
#include <chrono>

// l_port - port on wich server running
// handler - callback-function called when client connect. Example
// lambda-function: [](TcpServer::Client){...do something...})
TcpServer::TcpServer(const uint16_t port, handler_function_t handler)
    : m_port(port), m_handler(handler) {}

// Stop server if it was started
// and cleanup WinSocket
TcpServer::~TcpServer() {
  if (m_status == status::up)
    stop();
#ifdef _WIN32 // Windows NT
  WSACleanup();
#endif
}

// Set callback-function called when client connect
void TcpServer::setHandler(TcpServer::handler_function_t handler) {
  this->m_handler = handler;
}

// Port getter/setter
uint16_t TcpServer::getPort() const { return m_port; }
uint16_t TcpServer::setPort(const uint16_t port) {
  this->m_port = port;
  restart(); // restart if server running
  return port;
}

// Server restart
TcpServer::status TcpServer::restart() {
  if (m_status == status::up)
    stop();
  return start();
}

// Enter to connection processing thread
void TcpServer::joinLoop() { m_threadHandler.join(); }

// Load client data to buffer and return length
int TcpServer::Client::loadData() {
  return recv(socket, m_buffer, CLIENT_INPUT_BUFFER_SIZE, 0);
}
// Return pointer to buffer with client data
char *TcpServer::Client::getData() { return m_buffer; }

// Send data to client
bool TcpServer::Client::sendData(const char *buffer, const size_t size) const {
  if (send(socket, buffer, size, 0) < 0)
    return false;
  return true;
}

#ifdef _WIN32 // Windows NT
TcpServer::status TcpServer::start() {
  WSAStartup(MAKEWORD(2, 2), &w_data);
  SOCKADDR_IN address;
  address.sin_addr.S_un.S_addr = INADDR_ANY;
  address.sin_port = htons(port);
  address.sin_family = AF_INET;

  if (static_cast<int>(serv_socket = socket(AF_INET, SOCK_STREAM, 0)) ==
      SOCKET_ERROR)
    return _status = status::err_socket_init;

  if (bind(serv_socket, (struct sockaddr *)&address, sizeof(address)) ==
      SOCKET_ERROR)
    return _status = status::err_socket_bind;

  if (listen(serv_socket, SOMAXCONN) == SOCKET_ERROR)
    return _status = status::err_socket_listening;

  _status = status::up;
  handler_thread = std::thread([this] { handlingLoop(); });
  return _status;
}

void TcpServer::stop() {
  _status = status::close;
  closesocket(serv_socket);
  joinLoop();
  for (std::thread &cl_thr : client_handler_threads)
    cl_thr.join();
  client_handler_threads.clear();
  client_handling_end.clear();
}

void TcpServer::handlingLoop() {
  while (_status == status::up) {
    SOCKET client_socket;
    SOCKADDR_IN client_addr;
    int addrlen = sizeof(client_addr);

    if ((client_socket = accept(serv_socket, (struct sockaddr *)&client_addr,
                                &addrlen)) != 0 &&
        _status == status::up) {
      client_handler_threads.push_back(
          std::thread([this, &client_socket, &client_addr] {
            handler(Client(client_socket, client_addr));

            client_handling_end.push_back(std::this_thread::get_id());
          }));
    }

    if (!client_handling_end.empty())
      for (std::list<std::thread::id>::iterator id_it =
               client_handling_end.begin();
           !client_handling_end.empty(); id_it = client_handling_end.begin())
        for (std::list<std::thread>::iterator thr_it =
                 client_handler_threads.begin();
             thr_it != client_handler_threads.end(); ++thr_it)
          if (thr_it->get_id() == *id_it) {
            thr_it->join();
            client_handler_threads.erase(thr_it);
            client_handling_end.erase(id_it);
            break;
          }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

TcpServer::Client::Client(SOCKET socket, SOCKADDR_IN address)
    : socket(socket), address(address) {}

TcpServer::Client::Client(const TcpServer::Client &other)
    : socket(other.socket), address(other.address) {}

TcpServer::Client::~Client() {
  shutdown(socket, 0);
  closesocket(socket);
}

uint32_t TcpServer::Client::getHost() const {
  return address.sin_addr.S_un.S_addr;
}
uint16_t TcpServer::Client::getPort() const { return address.sin_port; }

#else // *nix

TcpServer::status TcpServer::start() {
  struct sockaddr_in server;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(m_port);
  server.sin_family = AF_INET;
  m_servSocket = socket(AF_INET, SOCK_STREAM, 0);

  int l_flag = 1;
  setsockopt(m_servSocket, SOL_SOCKET, SO_REUSEADDR, &l_flag, sizeof(l_flag));

  if (m_servSocket == -1)
    return m_status = status::err_socket_init;
  if (bind(m_servSocket, (struct sockaddr *)&server, sizeof(server)) < 0)
    return m_status = status::err_socket_bind;
  if (listen(m_servSocket, 3) < 0)
    return m_status = status::err_socket_listening;

  m_status = status::up;
  m_threadHandler = std::thread([this] { handlingLoop(); });
  return m_status;
}

// Stop server
void TcpServer::stop() {
  m_status = status::close;
  close(m_servSocket);
  joinLoop();
  for (std::thread &cl_thr : m_clientHandlerThreads)
    cl_thr.join();
  m_clientHandlerThreads.clear();
  m_clientHandlingEnd.clear();
}

void TcpServer::handlingLoop() {
  while (m_status == status::up) {
    int client_socket;
    struct sockaddr_in client_addr;
    int addrlen = sizeof(struct sockaddr_in);
    if ((client_socket = accept(m_servSocket, (struct sockaddr *)&client_addr,
                                (socklen_t *)&addrlen)) >= 0 &&
        m_status == status::up)
      m_clientHandlerThreads.push_back(
          std::thread([this, &client_socket, &client_addr] {
            m_handler(Client(client_socket, client_addr));
            m_clientHandlingEnd.push_back(std::this_thread::get_id());
          }));

    if (!m_clientHandlingEnd.empty())
      for (std::list<std::thread::id>::iterator id_it =
               m_clientHandlingEnd.begin();
           !m_clientHandlingEnd.empty(); id_it = m_clientHandlingEnd.begin())
        for (std::list<std::thread>::iterator thr_it =
                 m_clientHandlerThreads.begin();
             thr_it != m_clientHandlerThreads.end(); ++thr_it)
          if (thr_it->get_id() == *id_it) {
            thr_it->join();
            m_clientHandlerThreads.erase(thr_it);
            m_clientHandlingEnd.erase(id_it);
            break;
          }

    std::this_thread::sleep_for(std::chrono::milliseconds(SERVER_TASK_PERIOD));
  }
}

TcpServer::Client::Client(int socket, struct sockaddr_in address)
    : socket(socket), address(address) {}

TcpServer::Client::Client(const TcpServer::Client &other)
    : socket(other.socket), address(other.address) {}

TcpServer::Client::~Client() {
  shutdown(socket, 0);
  close(socket);
}

uint32_t TcpServer::Client::getHost() const { return address.sin_addr.s_addr; }
uint16_t TcpServer::Client::getPort() const { return address.sin_port; }

#endif
