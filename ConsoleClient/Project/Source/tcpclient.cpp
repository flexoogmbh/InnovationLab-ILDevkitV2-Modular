#include "tcpclient.hpp"

using namespace std;

TcpClient::TcpClient() {}

TcpClient::~TcpClient() {}

int TcpClient::scanNetwork() {
  int l_result = 0;
  int l_castSocket = 0;
  int l_broadcastEnable = 1;
  timeval l_rcvTimeout;

  sockaddr_in l_broadcastAddress;

  // char l_message[] = "IL-PassiveMatrix&Broadcast\n";
  // char l_response[] = "Broadcast";

  char *l_message = new char[RX_BUFFER_SIZE];
  char *l_dataIn = new char[RX_BUFFER_SIZE];
  char *l_tempBuffer = new char[RX_BUFFER_SIZE];

  char *l_dataPtr0 = nullptr;
  char *l_dataPtr1 = nullptr;

  memset(l_dataIn, 0, RX_BUFFER_SIZE);

  int l_recvLength = 0;

  if ((l_castSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("socket");
    l_result = -1;
    goto exit;
  }

  l_result = setsockopt(l_castSocket, SOL_SOCKET, SO_BROADCAST,
                        &l_broadcastEnable, sizeof(l_broadcastEnable));

  l_rcvTimeout.tv_sec = 1;
  l_rcvTimeout.tv_usec = 0;
  l_result = setsockopt(l_castSocket, SOL_SOCKET, SO_RCVTIMEO, &l_rcvTimeout,
                        sizeof(l_rcvTimeout));

  memset(&l_broadcastAddress, 0, sizeof(l_broadcastAddress));
  l_broadcastAddress.sin_family = AF_INET;
  l_broadcastAddress.sin_addr.s_addr = htonl(INADDR_BROADCAST);
  l_broadcastAddress.sin_port = htons(NETWORK_PORT);

  sprintf(l_message, "%s&%s\n", BOARDNAME, BROADCAST);

  if (sendto(l_castSocket, l_message, strlen(l_message), 0,
             (struct sockaddr *)&l_broadcastAddress,
             sizeof(l_broadcastAddress)) < 0) {
    perror("sendto");
    l_result = 1;
    goto exit;
  }

  sleep(1);
  l_recvLength = recv(l_castSocket, l_dataIn, RX_BUFFER_SIZE, 0);
  if (l_recvLength < 0) {
    perror("recvtimeout");
    l_result = 1;
    goto exit;
  }

  l_dataPtr0 = strstr(l_dataIn, "&");
  if (l_dataPtr0 == nullptr) {
    perror("server IP not found in response");
    l_result = 1;
    goto exit;
  }

  l_dataPtr1 = strstr(l_dataIn, ":");
  if (l_dataPtr1 == nullptr) {
    perror("server IP not found in response");
    l_result = 1;
    goto exit;
  }

  ++l_dataPtr0;
  memset(l_tempBuffer, 0, RX_BUFFER_SIZE);

  copy(l_dataPtr0, l_dataPtr1, l_tempBuffer);
  cout << "Found PassiveMatrix board" << endl;
  cout << "IP:" << l_tempBuffer << endl;
  m_serverAddress.sin_addr.s_addr = inet_addr(l_tempBuffer);

exit:
  delete[] l_message;
  delete[] l_dataIn;
  delete[] l_tempBuffer;

  return l_result;
}
