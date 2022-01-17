#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "tcpServer.hpp"
#include "usbclient.hpp"

#include <ctime>
#include <deque>
#include <errno.h>
#include <mutex>
#include <sstream>
#include <string>
#include <time.h>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

struct PassiveBoardSettings {
  uint16_t m_shiftX;
  uint16_t m_shiftY;
  uint16_t m_sizeX;
  uint16_t m_sizeY;
  uint16_t m_samplesNumber;
  uint16_t m_framerate;
  uint16_t m_adcSampleDelay;
  uint16_t m_offsetVoltage;
  uint16_t m_referenceVoltage;
  uint16_t m_filterType;
  PassiveBoardSettings();
};

struct Frame {
  size_t m_frameSize;
  time_t m_timestamp;
  uint32_t m_packageId;
  uint32_t *m_pFrame;
  Frame();
  ~Frame();
  int init(size_t l_frameSize);
};

struct PixelLink {
  uint8_t m_inX;
  uint8_t m_inY;

  PixelLink();
};

struct ConvTable {
  std::string m_matrixName;
  size_t m_xSize;
  size_t m_ySize;

  size_t m_xStart;
  size_t m_xEnd;
  size_t m_yStart;
  size_t m_yEnd;

  std::vector<PixelLink> m_table;
  ConvTable();
  ConvTable(size_t l_xSize, size_t l_ySize);
  int init(size_t l_xSize = 0, size_t l_ySize = 0);
  ~ConvTable();
};

enum class CommandState { NONE, REQUEST, WAIT, SUCCES, FAIL };

class Command {
public:
  Command(uint8_t l_commandCode, uint32_t l_timeout, uint16_t l_answerLength);
  ~Command();

  void setInactive();
  bool getExecuteState();
  void setExecuteState();
  bool getWaitState();
  void setWaitState();
  bool getSuccessState();
  void setSuccessState();
  bool getFailState();
  void setFailState();

  uint8_t getCommandCode();

  int setTimeout(uint32_t l_timeout);
  int timerTick(uint32_t l_time);

private:
  CommandState m_state;
  uint8_t m_commandCode;
  uint16_t m_answerLength;
  uint32_t m_timeout;
  uint32_t m_timeoutCounter;
};

enum class ApplicationState { INIT_SERVER, INIT_JSON, INIT_BOARD, GET_BAUD, WORK };
enum class ScanState { UNDEFINED, SCANNING, STOPED };
enum class BaudState { SWITCH_BAUD, NOT_SWITCH_BAUD };

class Application {
public:
  static Application *getInstance();
  int init(UsbClient *l_pUsbClient, int l_serverPort = 0);
  int run();
  int poll();
  int stop();
  int setDebug(bool l_val);
  int setJsonFile(char *l_jsonFile);
  int getBaudRate();

private:
  Application();
  ~Application();
  int commandsTimeoutTick(uint32_t l_timerTick);
  bool isWaitingCommands();
  uint8_t getExecuteCommand();
  void switchExecuteCommand();
  void checkErrorCommand();
  // TCP server
  static std::string getHostStr(const TcpServer::Client &client);
  static void message(TcpServer::Client l_client);
  int serverStart();
  static int prepareData(std::vector<char> *l_pHttpAnswer);
  static int pushToVector(std::vector<char> *l_pVector, const char *l_pString);
  // JSON
  int seekToNumber(char **l_pString);
  int loadJson();
  Frame *jsonConvert(Frame *l_pFrame);

  // Board functions
  int startComm();
  int startNoParamsComm();
  int stopComm();

  int getPassiveBoardSettings();
  int setPassiveBoardSettings();

  int getPassiveBoardVersion();

  int readData();
  bool checkPreamble(std::deque<uint8_t> *l_pInputBuffer);
  bool findSecondPreamble(std::deque<uint8_t> *l_pInputBuffer,
                          size_t &l_position);
  int dataParser();
  int answerParser(std::deque<uint8_t> *l_pData);
  int clearFrameBuffer();
  int frameParser(std::deque<uint8_t> *l_frame);

  // Variables
  int m_dataSocket = 0;
  uint8_t *m_pIncommingMessageBuffer = nullptr;

  ApplicationState m_applicationState = ApplicationState::INIT_SERVER;
  ScanState m_scanState = ScanState::UNDEFINED;
  BaudState m_baudState = BaudState::NOT_SWITCH_BAUD;

  std::vector<Command> m_command;

  bool m_jsonPresent : 1, m_debugMode : 1;

  char *m_jsonFile;

  TcpServer *m_pServer = nullptr;
  int m_serverPort;

  UsbClient *m_pUsbClient = nullptr;
  uint8_t *m_pWriteBuffer = nullptr;
  std::deque<uint8_t> *m_pInputBuffer = nullptr;
  uint8_t *m_pLinearDataBuffer = nullptr;

  uint8_t m_boardVersion[4];
  PassiveBoardSettings m_boardSettings;
  PassiveBoardSettings m_scanSettings;
  time_t m_scanTimestamp = 0;
  uint8_t m_bitResolution = 0;
  uint8_t m_requestStatus = 0;

  std::mutex m_FrameBufferLock;
  std::deque<Frame> m_pFrameBuffer;

  ConvTable m_jsonTable;

  static Application *m_pInstance;
};

#endif // APPLICATION_HPP
