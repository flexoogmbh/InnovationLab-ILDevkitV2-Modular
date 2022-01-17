#ifndef USB_CLIENT_HPP
#define USB_CLIENT_HPP

#include "settings.hpp"

#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>   //file controls
#include <termios.h> //POSIX terminal control definitions
#include <unistd.h>  //write(), read(), close()

class UsbClient {
public:
  UsbClient();
  ~UsbClient();
  int setPortName(char *l_portName);
  int detectDevice(uint8_t l_baudChange);
  int writeData(uint8_t *l_data, uint32_t l_length);
  int readData(uint8_t *l_data, uint32_t l_length);

private:
  int setupPort(int l_fd, int l_speed, int l_parity);
  void setBlocking(int l_fd, int l_shouldBlock);

  int m_serialPort;
  char *m_portName;
};

#endif // TCPCLIENT_HPP
