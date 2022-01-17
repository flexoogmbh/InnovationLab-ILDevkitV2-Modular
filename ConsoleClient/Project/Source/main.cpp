#include "main.hpp"
#include "application.hpp"
#include "tcpServer.hpp"
#include "tcpclient.hpp"
#include "usbclient.hpp"

#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <csignal>

using namespace std;

Application *g_pApplication;

int main(int argc, char **argv) {
  int l_result = 0;

  UsbClient l_usbclient;
  g_pApplication = Application::getInstance();
  int l_serverPort = 0;

  // Register system signals
  signal(SIGHUP, signalHandler);

  for (int i = 0; i < argc; ++i) {
    if (strstr(argv[i], "--debug")) {
      g_pApplication->setDebug(true);
    } else if (strstr(argv[i], "--help")) {
      cout << "InnovationLab Gmbh" << endl;
      cout << "passive matrix to HTTP driver v" << VERSION0 << "." << VERSION1
           << "." << VERSION2 << endl;
      cout << "--help - current help" << endl;
      cout << "--debug - run with debug log" << endl;
      cout << "--json filename.json - json file for convertation (default "
              "frame.json)"
           << endl;
      cout << "--port 5001 - TCP server port (5001 default)" << endl;
      cout << "--serial portname - name of serial port (default PassiveMatrix)"
           << endl;
      return 0;
    } else if (strstr(argv[i], "--json")) {
      g_pApplication->setJsonFile(argv[i + 1]);
    } else if (strstr(argv[i], "--port")) {
      l_serverPort = stoi(argv[i + 1]);
    } else if (strstr(argv[i], "--serial")) {
      l_usbclient.setPortName(argv[i + 1]);
    }
  }

  g_pApplication->init(&l_usbclient, l_serverPort);

  while (l_result == 0) {
    l_result = g_pApplication->run();
  }

  return l_result;
}

void signalHandler(int l_signum) {
  std::cout << "Interrupt signal (" << l_signum << ") received.\n";

  g_pApplication->stop();

  exit(l_signum);
}
