#include "usbclient.hpp"
#include "commands.hpp"
#include "application.hpp"

UsbClient::UsbClient() {
  m_portName = new char[MAX_SERIAL_PORT_NAME_LENGTH + 1];
  memset(m_portName, 0, MAX_SERIAL_PORT_NAME_LENGTH + 1);
  sprintf(m_portName, "%s", DEFAULT_SERIAL_PORT);
  m_serialPort = 0;
}

UsbClient::~UsbClient() { delete[] m_portName; }

int UsbClient::setPortName(char *l_portName) {
  memset(m_portName, 0, MAX_SERIAL_PORT_NAME_LENGTH + 1);
  strncpy(m_portName, l_portName, MAX_SERIAL_PORT_NAME_LENGTH);
  return 0;
}

int UsbClient::detectDevice(uint8_t l_baudChange) {
  int l_result = 0;

  DIR *l_pDirrectory = nullptr;
  dirent *l_pDirEntry = nullptr;
  char *l_devicePath = new char[PATH_LENGTH];

  memset(l_devicePath, 0, PATH_LENGTH);
  sprintf(l_devicePath, "%s", SERIAL_PATH);

  l_pDirrectory = opendir(l_devicePath);

  if (l_pDirrectory == nullptr) {
    l_result = errno;
    perror("serial port");
    goto exit;
  }

  if (l_pDirrectory) {
    while ((l_pDirEntry = readdir(l_pDirrectory)) != nullptr) {
      if (strstr(l_pDirEntry->d_name, m_portName)) {
        break;
      }
    }
  }

  if (l_pDirEntry != nullptr) {
    memset(l_devicePath, 0, PATH_LENGTH);
    sprintf(l_devicePath, "%s%s", SERIAL_PATH, l_pDirEntry->d_name);
    std::cout << l_devicePath << std::endl;
  } else {
    perror("file not found");
  }

  m_serialPort = open(l_devicePath, O_RDWR /*| O_NONBLOCK*/);

  if (m_serialPort <= 0) {
    l_result = errno;
    perror("serial port");
    goto exit;
  }

  switch(l_baudChange)
  {
    case 0:
      {
       setupPort(m_serialPort, B115200, 0);
       setBlocking(m_serialPort, 0);
       l_result = writeData((uint8_t*)SET_DATA_MODE, strlen(SET_DATA_MODE));
       sleep(1);
       if (l_result < 0) {
         l_result = errno;
         perror("serial port");
         goto exit;
       }else{
           l_result = 0;
       }

       std::cout << "DATA command sent" << std::endl;
       setupPort(m_serialPort, B1000000, 0);
       setBlocking(m_serialPort, 0);
      }break;
    case 1:
      {
       setupPort(m_serialPort, B1000000, 0);
       setBlocking(m_serialPort, 0);
      }break;
  }

exit:
  delete[] l_devicePath;
  return l_result;
}


int UsbClient::writeData(uint8_t *l_data, uint32_t l_length) {
  int l_result = 0;

  l_result = write(m_serialPort, l_data, l_length);

  usleep((l_length + 25) * 100);

  if (l_result < 0) {
    l_result = errno;
    perror("serial port");
    goto exit;
  }

exit:
  return l_result;
}

int UsbClient::readData(uint8_t *l_data, uint32_t l_length) {
  int l_result = 0;

  l_result = read(m_serialPort, l_data, l_length);

  if (l_result < 0) {
    l_result = errno;
    perror("serial port");
    goto exit;
  }

exit:
  return l_result;
}

int UsbClient::setupPort(int l_fd, int l_speed, int l_parity) {
  termios l_tty;
  memset(&l_tty, 0, sizeof(l_tty));

  if (tcgetattr(l_fd, &l_tty) != 0) {
    perror("error from tcgetattr");
    return -1;
  }

  cfsetospeed(&l_tty, l_speed);
  cfsetispeed(&l_tty, l_speed);

  l_tty.c_cflag = (l_tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
  // disable IGNBRK for mismatched speed tests; otherwise receive break
  // as \000 chars
  l_tty.c_iflag &= ~IGNBRK; // disable break processing
  l_tty.c_lflag = 0;        // no signaling chars, no echo,
                            // no canonical processing
  l_tty.c_oflag = 0;        // no remapping, no delays
  l_tty.c_cc[VMIN] = 0;     // read doesn't block
  l_tty.c_cc[VTIME] = 5;    // 0.5 seconds read timeout

  l_tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

  l_tty.c_cflag |= (CLOCAL | CREAD);   // ignore modem controls,
                                       // enable reading
  l_tty.c_cflag &= ~(PARENB | PARODD); // shut off parity
  l_tty.c_cflag |= l_parity;
  l_tty.c_cflag &= ~CSTOPB;
  l_tty.c_cflag &= ~CRTSCTS;

  if (tcsetattr(l_fd, TCSANOW, &l_tty) != 0) {
    perror("error from tcsetattr");
    return -1;
  }
  return 0;
}

void UsbClient::setBlocking(int l_fd, int l_shouldBlock) {
  termios l_tty;
  memset(&l_tty, 0, sizeof l_tty);
  if (tcgetattr(l_fd, &l_tty) != 0) {
    perror("error from tggetattr");
    return;
  }

  l_tty.c_cc[VMIN] = l_shouldBlock ? 1 : 0;
  l_tty.c_cc[VTIME] = 5; // 0.5 seconds read timeout

  if (tcsetattr(l_fd, TCSANOW, &l_tty) != 0)
    perror("error setting term attributes");
}
