#include "application.hpp"
#include "commands.hpp"

using namespace std;

/*PassiveBoardSettings*/
PassiveBoardSettings::PassiveBoardSettings() {
  m_shiftX = 0;
  m_shiftY = 0;
  m_sizeX = 0;
  m_sizeY = 0;
  m_samplesNumber = 0;
  m_framerate = 0;
  m_adcSampleDelay = 0;
  m_offsetVoltage = 0;
  m_referenceVoltage = 0;
  m_filterType = 0;
}

/*Frame*/

Frame::Frame() {
  m_frameSize = 0;
  m_timestamp = 0;
  m_packageId = 0;
  m_pFrame = nullptr;
}

Frame::~Frame() {
  delete[] m_pFrame;
  m_pFrame = nullptr;
}

int Frame::init(size_t l_frameSize) {
  m_frameSize = l_frameSize;
  m_pFrame = new uint32_t[l_frameSize];

  if (m_pFrame == nullptr) {
    return -1;
  } else {
    memset(m_pFrame, 0, l_frameSize);
    return 0;
  }
}

PixelLink::PixelLink() {
  m_inX = 0;
  m_inY = 0;
}

ConvTable::ConvTable() { init(0, 0); }

ConvTable::ConvTable(size_t l_xSize, size_t l_ySize) { init(l_xSize, l_ySize); }

ConvTable::~ConvTable() {}

int ConvTable::init(size_t l_xSize, size_t l_ySize) {
  m_xSize = l_xSize;
  m_ySize = l_ySize;

  m_xStart = MAX_X;
  m_xEnd = 0;
  m_yStart = MAX_Y;
  m_yEnd = 0;

  m_matrixName.clear();
  m_table.clear();
  return 0;
}

Command::Command(uint8_t l_commandCode, uint32_t l_timeout,
                 uint16_t l_answerLength) {
  m_state = CommandState::NONE;
  m_commandCode = l_commandCode;
  m_answerLength = l_answerLength;
  m_timeout = l_timeout;
  m_timeoutCounter = 0;
}

Command::~Command() {}

void Command::setInactive() { m_state = CommandState::NONE; }

bool Command::getExecuteState() {
  if (m_state == CommandState::REQUEST) {
    return true;
  } else {
    return false;
  }
}

void Command::setExecuteState() { m_state = CommandState::REQUEST; }

bool Command::getWaitState() {
  if (m_state == CommandState::WAIT) {
    return true;
  } else {
    return false;
  }
}

void Command::setWaitState() { m_state = CommandState::WAIT; }

bool Command::getSuccessState() {
  if (m_state == CommandState::SUCCES) {
    return true;
  } else {
    return false;
  }
}

void Command::setSuccessState() { m_state = CommandState::SUCCES; }

bool Command::getFailState() {
  if (m_state == CommandState::FAIL) {
    return true;
  } else {
    return false;
  }
}

void Command::setFailState() { m_state = CommandState::FAIL; }

uint8_t Command::getCommandCode() { return m_commandCode; }

int Command::setTimeout(uint32_t l_timeout) {
  m_timeout = l_timeout;
  return 0;
}

int Command::timerTick(uint32_t l_timeTick) {
  if (m_state == CommandState::WAIT) {
    if (m_timeoutCounter >= m_timeout) {
      m_state = CommandState::FAIL;
      m_timeoutCounter = 0;
    } else {
      m_timeoutCounter += l_timeTick;
    }
  }
  return 0;
}

/*Application*/
/*PUBLIC*/

Application *Application::getInstance() {

  if (m_pInstance == nullptr) {
    m_pInstance = new Application();
  }

  return m_pInstance;
}

int Application::init(UsbClient *l_pUsbClient, int l_serverPort) {
  m_dataSocket = 0;

  m_applicationState = ApplicationState::INIT_SERVER;
  m_scanState = ScanState::UNDEFINED;

  m_jsonPresent = false;
  m_debugMode = false;

  m_serverPort = DEFAULT_SERVER_PORT;

  if (l_serverPort) {
    m_serverPort = l_serverPort;
  }

  m_pServer = new TcpServer(m_serverPort, message);

  m_pUsbClient = l_pUsbClient;

  return 0;
}

/*Maint thread*/
int Application::run() {
  int l_rval = 0;

  switch (m_applicationState) {
  case ApplicationState::INIT_SERVER:
    if (serverStart() == 0) {
      m_applicationState = ApplicationState::INIT_JSON;
    } else {
      sleep(1);
    }
    break;
  case ApplicationState::INIT_JSON:
    if (loadJson() == 0) {
      m_applicationState = ApplicationState::INIT_BOARD;
    } else {
      sleep(1);
    }
    break;
  case ApplicationState::INIT_BOARD:
    if (m_pUsbClient->detectDevice((uint8_t)m_baudState) == 0) {
      m_applicationState = ApplicationState::GET_BAUD;
      m_command[COMM_ID_READ_FIRMWARE_VERSION].setExecuteState();
      //m_command[COMM_ID_STOP].setExecuteState();
    } else {
      sleep(1);
    }
    break;
  case ApplicationState::GET_BAUD:
     if (getBaudRate() != 0)
     {
       m_applicationState = ApplicationState::WORK;
       m_command[COMM_ID_STOP].setExecuteState();
     } else {
       sleep(1);
     }
    break;
  case ApplicationState::WORK:
    if (poll() < 0) {
      m_applicationState = ApplicationState::INIT_BOARD;
    }
    break;
  }

  usleep(APP_CYCLE_PERIOD);

  return l_rval;
}

int Application::getBaudRate() {
  int l_result = 0;

  commandsTimeoutTick(APP_DETECT_PERIOD);

  if (readData() < 0) {
    l_result = errno;
    perror("application");
    goto exit;
  }

  if (dataParser() < 0) {
    l_result = errno;
    perror("application");
    goto exit;
  }

  if (isWaitingCommands()) {
    goto exit;
  }

  if (m_command[COMM_ID_READ_FIRMWARE_VERSION].getFailState())
  {
      m_applicationState = ApplicationState::INIT_BOARD;
      m_baudState = BaudState::SWITCH_BAUD;
      std::cout << "Switching data rate..." << std::endl;
      goto exit;
  }
  if (m_command[COMM_ID_READ_FIRMWARE_VERSION].getSuccessState())
  {
      if (m_baudState == BaudState::SWITCH_BAUD)
      {
        std::cout << "Switching data rate successfully!" << std::endl;
      } else {
        std::cout << "No need to switch data rate!" << std::endl;
      }
      m_baudState = BaudState::NOT_SWITCH_BAUD;
      l_result = 1;
      goto exit;
  }

  if (getPassiveBoardVersion() == 0) {
    m_command[COMM_ID_READ_FIRMWARE_VERSION].setWaitState();
  }

exit:
  return l_result;
}

int Application::poll() {
  int l_rval = 0;

  commandsTimeoutTick(APP_CYCLE_PERIOD);

  if (readData() < 0) {
    l_rval = errno;
    perror("application");
    goto exit;
  }

  if (dataParser() < 0) {
    l_rval = errno;
    perror("application");
    goto exit;
  }

  if (isWaitingCommands()) {
    goto exit;
  }

  checkErrorCommand();

  switchExecuteCommand();

  // Make requests
  switch (getExecuteCommand()) {
  case COMM_ID_START:
    if (m_scanState == ScanState::SCANNING) {
      m_command[COMM_ID_START].setInactive();
      m_command[COMM_ID_STOP].setExecuteState();
    } else if (startComm()) {
      m_command[COMM_ID_START].setWaitState();
    }
    break;
  case COMM_ID_STOP:
    if (stopComm() == 0) {
      m_command[COMM_ID_STOP].setWaitState();
    }
    break;
  case COMM_ID_START_CAN:
    break;
  case COMM_ID_FRAME:
    break;
  case COMM_ID_STOP_CAN:
    break;
  case COMM_ID_CAN_READ_DEV_ID:
    break;
  case COMM_ID_CAN_WRITE_DEV_ID:
    break;
  case COMM_ID_WRITE_CONFIG:
    if (setPassiveBoardSettings() == 0) {
      m_command[COMM_ID_WRITE_CONFIG].setWaitState();
    }
    break;
  case COMM_ID_READ_CONFIG:
    if (getPassiveBoardSettings() == 0) {
      m_command[COMM_ID_READ_CONFIG].setWaitState();
    }
    break;
  case COMM_ID_READ_FIRMWARE_VERSION:
    if (getPassiveBoardVersion() == 0) {
      m_command[COMM_ID_READ_FIRMWARE_VERSION].setWaitState();
    }
    break;
  case COMM_ID_START_NO_PARAMS:
    if (m_scanState == ScanState::SCANNING) {
      m_command[COMM_ID_START_NO_PARAMS].setInactive();
      m_command[COMM_ID_STOP].setExecuteState();
    } else if (startNoParamsComm() == 0) {
      m_command[COMM_ID_START_NO_PARAMS].setWaitState();
    }
    break;
  case COMM_ID_RESET_JSON:
    break;
  case COMM_ID_WRITE_JSON:
    break;
  case COMM_ID_APPLY_JSON:
    break;
  case COMM_ID_GET_MATRIX_NAME:
    break;
  case COMM_ID_WRITE_WIFI_CONF:
    break;
  case COMM_ID_READ_WIFI_CONF:
    break;
  case COMM_ID_WRITE_FLTR_CONF:
    break;
  case COMM_ID_READ_FLTR_CONF:
    break;
  case COMM_ID_RESET_CALIBR:
    break;
  case COMM_ID_WRITE_CALIBR_REF:
    break;
  case COMM_ID_WRITE_CALIBR_TABLE:
    break;
  case COMM_ID_READ_CALIBR:
    break;
  case COMM_ID_READ_CALIBR_TABLE:
    break;
  case COMM_ID_READ_CALIBR_COMPLETE:
    break;
  case COMM_ID_WRITE_ACTIVE_CONF:
    break;
  case COMM_ID_READ_ACTIVE_CONF:
    break;
  }

exit:
  return l_rval;
}

int Application::stop() {
  int l_rval = 0;

  if (m_applicationState == ApplicationState::WORK) {
    if (m_scanState != ScanState::STOPED) {
      // m_stop = true;
    }

    while (m_scanState != ScanState::STOPED) {
      poll();
      usleep(APP_CYCLE_PERIOD);
    }
  }

  if (m_pServer != nullptr) {
    m_pServer->stop();
    m_pServer->joinLoop();
  }

  return l_rval;
}

int Application::setDebug(bool l_val) {
  m_debugMode = l_val;
  return 0;
}
int Application::setJsonFile(char *l_jsonFile) {
  memset(m_jsonFile, 0, MAX_JSON_NAME_LENGTH + 1);
  strncpy(m_jsonFile, l_jsonFile, MAX_JSON_NAME_LENGTH);
  return 0;
}
/*PRIVATE*/
Application::Application() {
  m_jsonFile = new char[MAX_JSON_NAME_LENGTH + 1];
  memset(m_jsonFile, 0, MAX_JSON_NAME_LENGTH + 1);
  sprintf(m_jsonFile, "%s", DEFAULT_JSON_FILE);

  m_pIncommingMessageBuffer = new uint8_t[INCOMMING_BUFFER_SIZE];

  m_pWriteBuffer = new uint8_t[100];
  m_pInputBuffer = new std::deque<uint8_t>;
  m_pLinearDataBuffer = new uint8_t[USB_PART_BUFFER_SIZE];

  m_command.push_back(Command(0, 0, 0));
  m_command.push_back(
      Command(COMM_ID_START, COMM_ID_START_TIMEOUT, START_ANSWER_LENGTH));
  m_command.push_back(
      Command(COMM_ID_STOP, COMM_ID_STOP_TIMEOUT, STOP_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_START_CAN, COMM_ID_START_CAN_TIMEOUT,
                              START_CAN_ANSWER_LENGTH));
  m_command.push_back(
      Command(COMM_ID_FRAME, COMM_ID_FRAME_TIMEOUT, FRAME_ANSRWER_LENGTH));
  m_command.push_back(
      Command(COMM_ID_STOP_CAN, COMM_ID_STOP_CAN_TIMEOUT, STOP_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_CAN_READ_DEV_ID,
                              COMM_ID_CAN_READ_DEV_ID_TIMEOUT,
                              CAN_READ_DEV_ID_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_CAN_WRITE_DEV_ID,
                              COMM_ID_CAN_WRITE_DEV_ID_TIMEOUT,
                              CAN_WRITE_DEV_ID_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_WRITE_CONFIG,
                              COMM_ID_WRITE_CONFIG_TIMEOUT,
                              WRITE_CONFIG_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_READ_CONFIG, COMM_ID_READ_CONFIG_TIMEOUT,
                              READ_CONFIG_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_READ_FIRMWARE_VERSION,
                              COMM_ID_READ_FIRMWARE_VERSION_TIMEOUT,
                              READ_FIRMWARE_VERSION_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_START_NO_PARAMS,
                              COMM_ID_START_NO_PARAMS_TIMEOUT,
                              START_NO_PARAMS_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_RESET_JSON, COMM_ID_RESET_JSON_TIMEOUT,
                              RESET_JSON_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_WRITE_JSON, COMM_ID_WRITE_JSON_TIMEOUT,
                              WRITE_JSON_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_APPLY_JSON, COMM_ID_APPLY_JSON_TIMEOUT,
                              APPLY_JSON_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_GET_MATRIX_NAME,
                              COMM_ID_GET_MATRIX_NAME_TIMEOUT,
                              GET_MATRIX_NAME_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_WRITE_WIFI_CONF,
                              COMM_ID_WRITE_WIFI_CONF_TIMEOUT,
                              WRITE_WIFI_CONF_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_READ_WIFI_CONF,
                              COMM_ID_READ_WIFI_CONF_TIMEOUT,
                              READ_WIFI_CONF_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_WRITE_FLTR_CONF,
                              COMM_ID_WRITE_FLTR_CONF_TIMEOUT,
                              WRITE_FLTR_CONF_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_READ_FLTR_CONF,
                              COMM_ID_READ_FLTR_CONF_TIMEOUT,
                              READ_FLTR_CONF_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_RESET_CALIBR,
                              COMM_ID_RESET_CALIBR_TIMEOUT,
                              RESET_CALIBR_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_WRITE_CALIBR_REF,
                              COMM_ID_WRITE_CALIBR_REF_TIMEOUT,
                              WRITE_CALIBR_REF_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_WRITE_CALIBR_TABLE,
                              COMM_ID_WRITE_CALIBR_TABLE_TIMEOUT,
                              WRITE_CALIBR_TABLE_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_READ_CALIBR,
                              COMM_ID_READ_CALIBR_COMPLETE_TIMEOUT,
                              READ_CALIBR_COMPLETE_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_READ_CALIBR_TABLE,
                              COMM_ID_READ_CALIBR_TABLE_TIMEOUT,
                              READ_CALIBR_TABLE_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_READ_CALIBR_COMPLETE,
                              COMM_ID_READ_CALIBR_COMPLETE_TIMEOUT,
                              READ_CALIBR_COMPLETE_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_WRITE_ACTIVE_CONF,
                              COMM_ID_WRITE_ACTIVE_CONF_TIMEOUT,
                              WRITE_ACTIVE_CONF_ANSWER_LENGTH));
  m_command.push_back(Command(COMM_ID_READ_ACTIVE_CONF,
                              COMM_ID_READ_ACTIVE_CONF_TIMEOUT,
                              READ_ACTIVE_CONF_ANSWER_LENGTH));
}

Application::~Application() {
  delete m_pServer;
  delete[] m_pIncommingMessageBuffer;
  delete[] m_pWriteBuffer;
  delete m_pInputBuffer;
  delete[] m_pLinearDataBuffer;
  delete[] m_jsonFile;
}

int Application::commandsTimeoutTick(uint32_t l_timerTick) {
  for (auto &l_command : m_command) {
    l_command.timerTick(l_timerTick);
  }
  return 0;
}

bool Application::isWaitingCommands() {
  for (auto &l_command : m_command) {
    if (l_command.getWaitState()) {
      return true;
    }
  }

  return false;
}

uint8_t Application::getExecuteCommand() {
  for (auto &l_command : m_command) {
    if (l_command.getExecuteState()) {
      return l_command.getCommandCode();
    }
  }

  return 0;
}

void Application::switchExecuteCommand() {
  if (m_command[COMM_ID_START].getSuccessState()) {
  } else if (m_command[COMM_ID_STOP].getSuccessState()) { // 1
    m_command[COMM_ID_STOP].setInactive();
    m_command[COMM_ID_READ_FIRMWARE_VERSION].setExecuteState();
  } else if (m_command[COMM_ID_START_CAN].getSuccessState()) {
  } else if (m_command[COMM_ID_FRAME].getSuccessState()) {
    m_command[COMM_ID_FRAME].setInactive();
  } else if (m_command[COMM_ID_STOP_CAN].getSuccessState()) {
  } else if (m_command[COMM_ID_CAN_READ_DEV_ID].getSuccessState()) {
  } else if (m_command[COMM_ID_CAN_WRITE_DEV_ID].getSuccessState()) {
  } else if (m_command[COMM_ID_WRITE_CONFIG].getSuccessState()) {
    m_command[COMM_ID_WRITE_CONFIG].setInactive();
    m_command[COMM_ID_READ_CONFIG].setExecuteState();
  } else if (m_command[COMM_ID_READ_CONFIG].getSuccessState()) { // 3
    m_command[COMM_ID_READ_CONFIG].setInactive();
    m_command[COMM_ID_START_NO_PARAMS].setExecuteState();
  } else if (m_command[COMM_ID_READ_FIRMWARE_VERSION].getSuccessState()) { // 2
    m_command[COMM_ID_READ_FIRMWARE_VERSION].setInactive();
    m_command[COMM_ID_READ_CONFIG].setExecuteState();
  } else if (m_command[COMM_ID_START_NO_PARAMS].getSuccessState()) { // 3
    m_command[COMM_ID_START_NO_PARAMS].setInactive();
  } else if (m_command[COMM_ID_RESET_JSON].getSuccessState()) {
  } else if (m_command[COMM_ID_WRITE_JSON].getSuccessState()) {
  } else if (m_command[COMM_ID_APPLY_JSON].getSuccessState()) {
  } else if (m_command[COMM_ID_GET_MATRIX_NAME].getSuccessState()) {
  } else if (m_command[COMM_ID_WRITE_WIFI_CONF].getSuccessState()) {
  } else if (m_command[COMM_ID_READ_WIFI_CONF].getSuccessState()) {
  } else if (m_command[COMM_ID_WRITE_FLTR_CONF].getSuccessState()) {
  } else if (m_command[COMM_ID_READ_FLTR_CONF].getSuccessState()) {
  } else if (m_command[COMM_ID_RESET_CALIBR].getSuccessState()) {
  } else if (m_command[COMM_ID_WRITE_CALIBR_REF].getSuccessState()) {
  } else if (m_command[COMM_ID_WRITE_CALIBR_TABLE].getSuccessState()) {
  } else if (m_command[COMM_ID_READ_CALIBR].getSuccessState()) {
  } else if (m_command[COMM_ID_READ_CALIBR_TABLE].getSuccessState()) {
  } else if (m_command[COMM_ID_READ_CALIBR_COMPLETE].getSuccessState()) {
  } else if (m_command[COMM_ID_WRITE_ACTIVE_CONF].getSuccessState()) {
  } else if (m_command[COMM_ID_READ_ACTIVE_CONF].getSuccessState()) {
  }
}

void Application::checkErrorCommand() {
  for (auto &l_command : m_command) {
    if (l_command.getFailState()) {
      l_command.setExecuteState();
    }
  }
}

//Парсер ip в std::string
std::string Application::getHostStr(const TcpServer::Client &client) {
  uint32_t ip = client.getHost();
  return std::string() + std::to_string(int(reinterpret_cast<char *>(&ip)[0])) +
         '.' + std::to_string(int(reinterpret_cast<char *>(&ip)[1])) + '.' +
         std::to_string(int(reinterpret_cast<char *>(&ip)[2])) + '.' +
         std::to_string(int(reinterpret_cast<char *>(&ip)[3])) + ':' +
         std::to_string(client.getPort());
}

void Application::message(TcpServer::Client l_client) {
  std::vector<char> l_pHttpAnswer;
  // Connectel client
  if (m_pInstance->m_debugMode) {
    std::cout << "Connected host:" << getHostStr(l_client) << std::endl;
  }

  // Get data from client
  size_t l_size = 0;
  while (!(l_size = l_client.loadData())) {
    usleep(100);
  }

  // Output recieved data to console
  if (m_pInstance->m_debugMode) {
    std::cout << "size: " << l_size << " bytes" << std::endl
              << l_client.getData() << std::endl;
  }
  if (m_pInstance->m_pFrameBuffer.size() == 0) {
    goto exit;
  }

  prepareData(&l_pHttpAnswer);

  // Send responce to client
  if (l_client.sendData((const char *)&l_pHttpAnswer[0],
                        l_pHttpAnswer.size())) {
  }

exit:
  return;
}

int Application::serverStart() {

  // Start server
  if (m_pServer->start() == TcpServer::status::up) {
    // Message if server is running and enter to the waiting stream
    std::cout << "Server is up!" << std::endl;
  } else {
    // Error if server do not running
    std::cout << "Server start error! Error code:"
              << int(m_pServer->getStatus()) << std::endl;
    return -1;
  }
  return 0;
}

int Application::prepareData(std::vector<char> *l_pHttpAnswer) {
  int l_rval = 0;
  const int l_tempLength = 200;
  char l_tempStr[l_tempLength];

  size_t l_maxX = 0;
  size_t l_maxY = 0;

  Frame *l_pFrame;
  std::vector<char> l_dataBuffer;

  lock_guard<mutex> lock(m_pInstance->m_FrameBufferLock);

  // If data present - take it, else - send empty buffer
  if (m_pInstance->m_pFrameBuffer.size() != 0) {
    l_pFrame = &m_pInstance->m_pFrameBuffer.at(0);
    l_maxX = m_pInstance->m_scanSettings.m_sizeX;
    l_maxY = m_pInstance->m_scanSettings.m_sizeY;
  } else {
    l_pFrame = new Frame;
    l_pFrame->init(MAX_X * MAX_Y);
    l_maxX = MAX_X;
    l_maxY = MAX_Y;
  }

  pushToVector(&l_dataBuffer, "[");

  for (size_t x = 0; x < l_maxX; ++x) {

    pushToVector(&l_dataBuffer, "[");

    for (size_t y = 0; y < l_maxY; ++y) {
      memset(l_tempStr, 0, l_tempLength);
      sprintf(l_tempStr, "%d,", l_pFrame->m_pFrame[x * l_maxY + y]);
      pushToVector(&l_dataBuffer, l_tempStr);
    }
    l_dataBuffer.pop_back(); // extract last ","
    pushToVector(&l_dataBuffer, "],");
  }

  l_dataBuffer.pop_back(); // extract last ","

  pushToVector(&l_dataBuffer, "]");

  l_pHttpAnswer->clear();

  pushToVector(l_pHttpAnswer, "HTTP/1.1 200 OK\r\n");

  memset(l_tempStr, 0, l_tempLength);
  sprintf(l_tempStr, "Content - Length : %i \r\n",
          static_cast<int>(l_dataBuffer.size()));
  pushToVector(l_pHttpAnswer, l_tempStr);

  pushToVector(l_pHttpAnswer, "Content-Type: application/json\r\n");

  memset(l_tempStr, 0, l_tempLength);
  sprintf(l_tempStr, "Server: IL-LinuxDataServer v%d\.%d\.%d \r\n", VERSION0,
          VERSION1, VERSION2); // Server: Microsoft-HTTPAPI/2.0
  pushToVector(l_pHttpAnswer, l_tempStr);

  pushToVector(l_pHttpAnswer, "Date: Wed, 14 Jul 2021 13:48:29 GMT");

  pushToVector(l_pHttpAnswer, "\r\n\r\n");

  for_each(l_dataBuffer.begin(), l_dataBuffer.end(),
           [l_pHttpAnswer](char l_byte) { l_pHttpAnswer->push_back(l_byte); });

  if (m_pInstance->m_pFrameBuffer.size() != 0) {
    m_pInstance->m_pFrameBuffer.pop_front();
  } else {
    delete l_pFrame;
  }

exit:
  return l_rval;
}

int Application::pushToVector(std::vector<char> *l_pVector,
                              const char *l_pString) {
  int l_rval = 0;

  while (*l_pString) {
    l_pVector->push_back(*l_pString);
    ++l_pString;
  }

  return l_rval;
}

/*Load JSON and parse to table*/

int Application::seekToNumber(char **l_pString) {
  while (1) {
    if (**l_pString == 0) {
      return -1;
    } else if ((**l_pString >= 0x30) && (**l_pString <= 0x39)) {
      return 0;
    } else {
      ++(*l_pString);
    }
  }
}

int Application::loadJson() {
  int l_rval = 0;
  int l_jsonFileDescriptor = 0;

  vector<char> l_fileBuffer;
  char l_byte;
  size_t l_readedBytes = 0;

  char *l_stringStart = nullptr;
  char *l_stringEnd = nullptr;
  PixelLink l_link;

  // Open file
  l_jsonFileDescriptor = open(m_jsonFile, O_RDONLY);

  if (l_jsonFileDescriptor <= 0) {
    l_rval = errno;
    perror("json file");
    goto exit;
  }

  cout << m_jsonFile << " opened" << endl;

  do {
    l_readedBytes = read(l_jsonFileDescriptor, &l_byte, 1);
    if (l_readedBytes == 1) {
      l_fileBuffer.push_back(l_byte);
    } else {
      break;
    }
  } while (1);

  close(l_jsonFileDescriptor);

  l_stringStart = strstr(&l_fileBuffer[0], "\"Name\":");

  if (l_stringStart == nullptr) {
    cout << "No JSON found" << endl;
    l_rval = -1;
    goto exit;
  }

  l_stringStart += 7;
  l_stringStart = strstr(l_stringStart, "\"");
  if (l_stringStart == nullptr) {
    l_rval = -1;
    goto exit;
  }
  ++l_stringStart;
  l_stringEnd = strstr(l_stringStart, "\"");
  if (l_stringEnd == nullptr) {
    l_rval = -1;
    goto exit;
  }

  m_jsonTable.init(); // reset conversion object

  for (char *i = l_stringStart; i != l_stringEnd; ++i) {
    m_jsonTable.m_matrixName.push_back(*i);
  }

  cout << "Matrix name in JSON file: " << m_jsonTable.m_matrixName << endl;

  // Parse matrix size

      //DriveLines
      l_stringStart = strstr(&l_fileBuffer[0], "\"DriveLines\":");

      if (l_stringStart == nullptr) {
          l_rval = -1;
          goto exit;
      }

      l_stringStart += strlen("\"DriveLines\":");
      m_jsonTable.m_xSize = atoi(l_stringStart);

      //Scanlines
      l_stringStart = strstr(&l_fileBuffer[0], "\"ScanLines\":");

      if (l_stringStart == nullptr) {
          l_rval = -1;
          goto exit;
      }

      l_stringStart += strlen("\"ScanLines\":");
      m_jsonTable.m_ySize = atoi(l_stringStart);


  if ((m_jsonTable.m_xSize > MAX_X) || (m_jsonTable.m_ySize > MAX_Y)) {
    l_rval = -1;
    goto exit;
  }

  cout << "JSON size X: " << m_jsonTable.m_xSize << endl;
  cout << "JSON size Y: " << m_jsonTable.m_ySize << endl;

  // Parse json to matrix conversion table
  // Search PCM token
  l_stringStart = strstr(&l_fileBuffer[0], "\"PCM\":");
  if (l_stringStart == nullptr) {
    l_rval = -1;
    goto exit;
  }

  l_stringStart += 6;
  //l_stringStart += strlen("\"PCM\":");

  for (uint16_t l_x = 0; l_x < m_jsonTable.m_xSize; l_x++) {
    for (uint16_t l_y = 0; l_y < m_jsonTable.m_ySize; l_y++) {

      if (seekToNumber(&l_stringStart) < 0) {
        l_rval = -1;
        goto exit;
      }

      l_link.m_inX = atoi(l_stringStart);

      if (m_jsonTable.m_xStart > l_link.m_inX) {
        m_jsonTable.m_xStart = l_link.m_inX;
      }
      if (m_jsonTable.m_xEnd < l_link.m_inX) {
        m_jsonTable.m_xEnd = l_link.m_inX;
      }

      l_stringStart = strstr(l_stringStart, ",");
      ++l_stringStart;

      if (l_stringStart == nullptr) {
        l_rval = -1;
        goto exit;
      }

      l_link.m_inY = atoi(l_stringStart);

      if (m_jsonTable.m_yStart > l_link.m_inY) {
        m_jsonTable.m_yStart = l_link.m_inY;
      }
      if (m_jsonTable.m_yEnd < l_link.m_inY) {
        m_jsonTable.m_yEnd = l_link.m_inY;
      }

      l_stringStart = strstr(l_stringStart, "]");
      ++l_stringStart;

      m_jsonTable.m_table.push_back(l_link);
    }
  }

  if ((m_jsonTable.m_xSize == (m_jsonTable.m_xEnd - m_jsonTable.m_xStart + 1)) &&
      (m_jsonTable.m_ySize == (m_jsonTable.m_yEnd - m_jsonTable.m_yStart + 1)))
      {
          cout << "JSON: Matrix JSON parsed successfully" << endl;
          m_jsonPresent = true;
      }else {
          cout << "JSON: Matrix JSON size in DriveLines or ScanLines mismatch with real coordinates" << endl;
          m_jsonPresent = false;
      }

  //m_jsonPresent = true;

exit:
  return l_rval;
}

Frame *Application::jsonConvert(Frame *l_pFrame) {
  uint8_t l_row = 0;
  uint8_t l_column = 0;

  if (m_jsonPresent) {
    Frame *l_pOutFrame = new Frame;
    l_pOutFrame->init(l_pFrame->m_frameSize);
    l_pOutFrame->m_packageId = l_pFrame->m_packageId;
    l_pOutFrame->m_timestamp = l_pFrame->m_timestamp;

    for (size_t x = 0; x < m_jsonTable.m_xSize; x++) {
      for (size_t y = 0; y < m_jsonTable.m_ySize; y++) {

        l_column = m_jsonTable.m_table[(x * m_jsonTable.m_ySize) + y].m_inX;
        l_row = m_jsonTable.m_table[(x * m_jsonTable.m_ySize) + y].m_inY;

        l_pOutFrame->m_pFrame[(x * m_jsonTable.m_ySize) + y] =
            l_pFrame->m_pFrame[(l_column * m_jsonTable.m_ySize) + l_row];
      }
    }
    delete l_pFrame;
    return l_pOutFrame;
  } else {
    return l_pFrame;
  }
}

/*Send start scanning command*/
int Application::startComm() {
  int l_rval = 0;
  uint16_t l_index = 0;

  // Preamble
  m_pWriteBuffer[l_index++] = 0xFF;
  m_pWriteBuffer[l_index++] = 0xFF;
  m_pWriteBuffer[l_index++] = 0xFF;
  m_pWriteBuffer[l_index++] = 0xFF;

  // Divider
  m_pWriteBuffer[l_index++] = 0x00;

  // 5-6 Data length
  m_pWriteBuffer[l_index++] = 0x0C;
  m_pWriteBuffer[l_index++] = 0x00;

  // Divider
  m_pWriteBuffer[l_index++] = 0x00;

  // Command code
  m_pWriteBuffer[l_index++] = COMM_ID_START;

  // Shifts
  m_pWriteBuffer[l_index++] = static_cast<uint8_t>(m_boardSettings.m_shiftX);
  m_pWriteBuffer[l_index++] = static_cast<uint8_t>(m_boardSettings.m_shiftY);
  // Sizes
  m_pWriteBuffer[l_index++] = static_cast<uint8_t>(m_boardSettings.m_sizeX);
  m_pWriteBuffer[l_index++] = static_cast<uint8_t>(m_boardSettings.m_sizeY);
  // Number of samples
  m_pWriteBuffer[l_index++] =
      static_cast<uint8_t>(m_boardSettings.m_samplesNumber);
  // Framerate
  m_pWriteBuffer[l_index++] =
      static_cast<uint8_t>(m_boardSettings.m_framerate >> 8);
  m_pWriteBuffer[l_index++] =
      static_cast<uint8_t>(m_boardSettings.m_framerate >> 0);
  // Sample delay
  m_pWriteBuffer[l_index++] =
      static_cast<uint8_t>(m_boardSettings.m_adcSampleDelay >> 8);
  m_pWriteBuffer[l_index++] =
      static_cast<uint8_t>(m_boardSettings.m_adcSampleDelay >> 0);

  if (m_pUsbClient->writeData(m_pWriteBuffer, l_index) != l_index) {
    l_rval = errno;
    perror("application");
    goto exit;
  };

  m_command[COMM_ID_START].setWaitState();

exit:
  return l_rval;
}

/*Send start command without parameters*/
int Application::startNoParamsComm() {
  int l_rval = 0;
  uint16_t l_index = 0;
  // Preamble
  m_pWriteBuffer[l_index++] = 0xFF;
  m_pWriteBuffer[l_index++] = 0xFF;
  m_pWriteBuffer[l_index++] = 0xFF;
  m_pWriteBuffer[l_index++] = 0xFF;

  // Divider
  m_pWriteBuffer[l_index++] = 0x00;

  // 5-6 Data length
  m_pWriteBuffer[l_index++] = 0x02;
  m_pWriteBuffer[l_index++] = 0x00;

  // Divider
  m_pWriteBuffer[l_index++] = 0x00;

  // Command code
  m_pWriteBuffer[l_index++] = COMM_ID_START_NO_PARAMS;

  if (m_pUsbClient->writeData(m_pWriteBuffer, l_index) != l_index) {
    l_rval = errno;
    perror("application");
    goto exit;
  };

  m_command[COMM_ID_START_NO_PARAMS].setWaitState();

exit:
  return l_rval;
}

/*Send stop command*/
int Application::stopComm() {
  int l_rval = 0;
  uint16_t l_index = 0;

  // Preamble
  m_pWriteBuffer[l_index++] = 0xFF;
  m_pWriteBuffer[l_index++] = 0xFF;
  m_pWriteBuffer[l_index++] = 0xFF;
  m_pWriteBuffer[l_index++] = 0xFF;

  // Divider
  m_pWriteBuffer[l_index++] = 0x00;

  // 5-6 Data length
  m_pWriteBuffer[l_index++] = 0x02;
  m_pWriteBuffer[l_index++] = 0x00;

  // Divider
  m_pWriteBuffer[l_index++] = 0x00;

  // Command code
  m_pWriteBuffer[l_index++] = COMM_ID_STOP;

  if (m_pUsbClient->writeData(m_pWriteBuffer, l_index) != l_index) {
    l_rval = errno;
    perror("application");
    goto exit;
  };

  m_command[COMM_ID_STOP].setWaitState();

exit:
  return l_rval;
}

/*Function make request for board ettings and store it in to internal class
 * variable*/
int Application::getPassiveBoardSettings() {
  int l_rval = 0;
  uint16_t l_index = 0;

  // Preamble
  m_pWriteBuffer[l_index++] = 0xFF;
  m_pWriteBuffer[l_index++] = 0xFF;
  m_pWriteBuffer[l_index++] = 0xFF;
  m_pWriteBuffer[l_index++] = 0xFF;

  // Divider
  m_pWriteBuffer[l_index++] = 0x00;

  // 5-6 Data length
  m_pWriteBuffer[l_index++] = 0x02;
  m_pWriteBuffer[l_index++] = 0x00;

  // Divider
  m_pWriteBuffer[l_index++] = 0x00;

  // Command code
  m_pWriteBuffer[l_index++] = COMM_ID_READ_CONFIG;

  if (m_pUsbClient->writeData(m_pWriteBuffer, l_index) != l_index) {
    l_rval = errno;
    perror("application");
    goto exit;
  };

  m_command[COMM_ID_READ_CONFIG].setWaitState();

exit:
  usleep(10000);
  return l_rval;
}

/*Function make request for write board settings*/
int Application::setPassiveBoardSettings() {
  int l_rval = 0;
  uint16_t l_index = 0;

  // Preamble
  m_pWriteBuffer[l_index++] = 0xFF;
  m_pWriteBuffer[l_index++] = 0xFF;
  m_pWriteBuffer[l_index++] = 0xFF;
  m_pWriteBuffer[l_index++] = 0xFF;

  // Divider
  m_pWriteBuffer[l_index++] = 0x00;

  // 5-6 Data length
  m_pWriteBuffer[l_index++] = 0x12;
  m_pWriteBuffer[l_index++] = 0x00;

  // Divider
  m_pWriteBuffer[l_index++] = 0x00;

  // Command code
  m_pWriteBuffer[l_index++] = COMM_ID_WRITE_CONFIG;

  // ShiftX
  m_pWriteBuffer[l_index++] = m_scanSettings.m_shiftX;
  // ShiftY
  m_pWriteBuffer[l_index++] = m_scanSettings.m_shiftY;
  // LengthX
  m_pWriteBuffer[l_index++] = m_scanSettings.m_sizeX;
  // LengthY
  m_pWriteBuffer[l_index++] = m_scanSettings.m_sizeY;
  // SampleNumber
  m_pWriteBuffer[l_index++] = m_scanSettings.m_samplesNumber;
  // UpdateFrequencyL
  m_pWriteBuffer[l_index++] = m_scanSettings.m_framerate;
  // UpdateFrequencyH
  m_pWriteBuffer[l_index++] = m_scanSettings.m_framerate >> 8;
  // Divider
  m_pWriteBuffer[l_index++] = 0;
  // ADC delayL
  m_pWriteBuffer[l_index++] = m_scanSettings.m_adcSampleDelay;
  // ADC delayH
  m_pWriteBuffer[l_index++] = m_scanSettings.m_adcSampleDelay >> 8;
  // Divider
  m_pWriteBuffer[l_index++] = 0;
  // Offset VoltageL
  m_pWriteBuffer[l_index++] = m_scanSettings.m_offsetVoltage;
  // Offset VoltageH
  m_pWriteBuffer[l_index++] = m_scanSettings.m_offsetVoltage >> 8;
  // Refference VoltageL
  m_pWriteBuffer[l_index++] = m_scanSettings.m_referenceVoltage;
  // Refference VoltageL
  m_pWriteBuffer[l_index++] = m_scanSettings.m_referenceVoltage >> 8;
  // FilterType
  m_pWriteBuffer[l_index++] = m_scanSettings.m_filterType;

  if (m_pUsbClient->writeData(m_pWriteBuffer, l_index) != l_index) {
    l_rval = errno;
    perror("application");
    goto exit;
  };

exit:
  usleep(10000);
  return l_rval;
}

/*Function make request for firmware verfion and store it in to internal class
 * variable*/
int Application::getPassiveBoardVersion() {
  int l_rval = 0;
  uint16_t l_index = 0;

  // Preamble
  m_pWriteBuffer[l_index++] = 0xFF;
  m_pWriteBuffer[l_index++] = 0xFF;
  m_pWriteBuffer[l_index++] = 0xFF;
  m_pWriteBuffer[l_index++] = 0xFF;

  // Divider
  m_pWriteBuffer[l_index++] = 0x00;

  // 5-6 Data length
  m_pWriteBuffer[l_index++] = 0x02;
  m_pWriteBuffer[l_index++] = 0x00;

  // Divider
  m_pWriteBuffer[l_index++] = 0x00;

  // Command code
  m_pWriteBuffer[l_index++] = COMM_ID_READ_FIRMWARE_VERSION;
  if (m_pUsbClient->writeData(m_pWriteBuffer, l_index) != l_index) {
    l_rval = errno;
    perror("application");
    goto exit;
  };

exit:
  return l_rval;
}

/*Do request for new data*/
int Application::readData() {
  int l_rval = 0;

  l_rval = m_pUsbClient->readData(m_pLinearDataBuffer, USB_PART_BUFFER_SIZE);

  if (l_rval < 0) {
    l_rval = errno;
    perror("serial port");
    goto exit;
  }

  if (l_rval == 0) {
    goto exit;
  }

  for (uint16_t i = 0; i < l_rval; i++) {
    m_pInputBuffer->push_back(m_pLinearDataBuffer[i]);
  }

exit:
  return l_rval;
}

/*Function is used for chek preamble in buffer*/
bool Application::checkPreamble(deque<uint8_t> *l_pInputBuffer) {
  if (l_pInputBuffer->size() < 9) {
    // cout<<"Not enough data");
    return false;
  } else if ((l_pInputBuffer->at(0) == PREAMBLE0) &&
             (l_pInputBuffer->at(1) == PREAMBLE1) &&
             (l_pInputBuffer->at(2) == PREAMBLE2) &&
             (l_pInputBuffer->at(3) == PREAMBLE3) &&
             (l_pInputBuffer->at(4) == 0x00) &&
             ((l_pInputBuffer->at(5) != 0x00) ||
              (l_pInputBuffer->at(6) != 0x00)) &&
             (l_pInputBuffer->at(7) == 0x00) &&
             (l_pInputBuffer->at(8) != 0x00)) {
    // cout<<"Preamble ok");
    return true;
  } else {
    // cout<<"Bad preamble");
    return false;
  }
}

/*Check if another preamble in buffer present*/
bool Application::findSecondPreamble(deque<uint8_t> *l_pInputBuffer,
                                     size_t &l_position) {
  // Check if length is enough for second preamble:
  // preamble(4) + divider(1) + size(2)+ preamble(4) = 11
  if (l_pInputBuffer->size() < 11) {
    return false;
  }

  for (size_t i = 7; i < (l_pInputBuffer->size() - 4); i++) {
    if ((l_pInputBuffer->at(i + 0) == PREAMBLE0) &&
        (l_pInputBuffer->at(i + 1) == PREAMBLE1) &&
        (l_pInputBuffer->at(i + 2) == PREAMBLE2) &&
        (l_pInputBuffer->at(i + 3) == PREAMBLE3)) {
      l_position = i;
      return true;
    }
  }
  return false;
}

/*Parse buffer*/
int Application::dataParser() {
  int l_rval = 0;

  while (m_pInputBuffer->size() >= 7) {
    if (!checkPreamble(m_pInputBuffer)) {
      m_pInputBuffer->pop_front();
    } else {
      break;
    }
  }

  // Check minimal length of the collected data: preamble(4)+size(2)=6
  if (m_pInputBuffer->size() <= 7) {
    // Data size no enough. Return
    goto exit;
  }

  answerParser(m_pInputBuffer);
exit:
  return l_rval;
}

/*Parse buffer for the answers from server*/
int Application::answerParser(deque<uint8_t> *l_pData) {
  int l_rval = 0;
  size_t l_dataLength = 0;
  size_t l_position = 0;
  uint8_t l_command = 0;
  // Parse answer
  // Check preamble

  if (!checkPreamble(l_pData)) {
    errno = EBADMSG;
    l_rval = errno;
    perror("application");
    goto exit;
  }

  // check divider
  if ((l_pData->at(4) != 0x00)) {
    errno = EBADMSG;
    l_rval = errno;
    perror("application");
    goto exit;
  }

  // Get data length
  l_dataLength = (static_cast<uint16_t>(l_pData->at(6)) << 8) |
                 (static_cast<uint16_t>(l_pData->at(5)));

  // Check if all data collected
  if (l_pData->size() < (l_dataLength + 7)) {
    // Size to small. Check for second preamble
    if (findSecondPreamble(l_pData, l_position)) {
      // Next preamble found. Clear buffer
      for (size_t i = 0; i < l_position; i++) {
        l_pData->pop_front();
      }
    }
    // errno = EBADMSG;
    // l_rval = errno;
    // perror("application");
    goto exit;
  }

  // get command code
  l_command = l_pData->at(8);

  switch (l_command) {
  case COMM_ID_START:
    m_command[COMM_ID_START].setSuccessState();
    break;
  case COMM_ID_STOP:
    cout << "COMM_ID_STOP answer" << endl;
    if (l_pData->at(9) == 0) {
      cout << "Scan stopped" << endl;
      clearFrameBuffer();
      m_scanState = ScanState::STOPED;
      m_command[COMM_ID_STOP].setSuccessState();
    }
    break;
  case COMM_ID_START_CAN:
    m_command[COMM_ID_START_CAN].setSuccessState();
    break;
  case COMM_ID_FRAME:
    frameParser(l_pData);
    m_command[COMM_ID_FRAME].setSuccessState();
    break;
  case COMM_ID_STOP_CAN:
    cout << "COMM_ID_STOP answer" << endl;
    if (l_pData->at(8) == 0) {
      cout << "Scan stopped" << endl;
      clearFrameBuffer();
      m_scanState = ScanState::STOPED;
      m_command[COMM_ID_STOP_CAN].setSuccessState();
    }
    break;
  case COMM_ID_CAN_READ_DEV_ID:
    m_command[COMM_ID_CAN_READ_DEV_ID].setSuccessState();
    break;
  case COMM_ID_CAN_WRITE_DEV_ID:
    m_command[COMM_ID_CAN_WRITE_DEV_ID].setSuccessState();
    break;
  case COMM_ID_WRITE_CONFIG:
    m_command[COMM_ID_WRITE_CONFIG].setSuccessState();
    break;
  case COMM_ID_READ_CONFIG:
    cout << "COMM_ID_READ_CONFIG answer" << endl;
    m_boardSettings.m_shiftX = l_pData->at(9);
    cout << "ShiftX=" << static_cast<uint16_t>(m_boardSettings.m_shiftX)
         << endl;
    m_boardSettings.m_shiftY = l_pData->at(10);
    cout << "ShiftY=" << static_cast<uint16_t>(m_boardSettings.m_shiftY)
         << endl;
    m_boardSettings.m_sizeX = l_pData->at(11);
    cout << "SizeX=" << static_cast<uint16_t>(m_boardSettings.m_sizeX) << endl;
    m_boardSettings.m_sizeY = l_pData->at(12);
    cout << "SizeY=" << static_cast<uint16_t>(m_boardSettings.m_sizeY) << endl;
    m_boardSettings.m_samplesNumber = l_pData->at(13);
    cout << "SamplesNumber="
         << static_cast<uint16_t>(m_boardSettings.m_samplesNumber) << endl;
    m_boardSettings.m_framerate =
        (static_cast<uint16_t>(l_pData->at(15) << 8)) |
        (static_cast<uint16_t>(l_pData->at(14)));
    cout << "FrameRate=" << static_cast<uint16_t>(m_boardSettings.m_framerate)
         << endl;
    m_boardSettings.m_adcSampleDelay =
        (static_cast<uint16_t>(l_pData->at(18) << 8)) |
        (static_cast<uint16_t>(l_pData->at(17)));
    cout << "AdcDelay="
         << static_cast<uint16_t>(m_boardSettings.m_adcSampleDelay) << endl;
    m_boardSettings.m_offsetVoltage =
        (static_cast<uint16_t>(l_pData->at(21) << 8)) |
        (static_cast<uint16_t>(l_pData->at(20)));
    cout << "OffsetVoltage="
         << static_cast<uint16_t>(m_boardSettings.m_offsetVoltage) << endl;
    m_boardSettings.m_referenceVoltage =
        (static_cast<uint16_t>(l_pData->at(23) << 8)) |
        (static_cast<uint16_t>(l_pData->at(22)));
    cout << "ReferenceVoltage="
         << static_cast<uint16_t>(m_boardSettings.m_referenceVoltage) << endl;
    m_boardSettings.m_filterType = l_pData->at(24);
    cout << "FilterType=" << static_cast<uint16_t>(m_boardSettings.m_filterType)
         << endl;
    m_command[COMM_ID_READ_CONFIG].setSuccessState();
    break;
  case COMM_ID_READ_FIRMWARE_VERSION:
    cout << "COMM_ID_READ_FIRMWARE_VERSION answer" << endl;
    if (l_dataLength != 0x07) {
      errno = EBADMSG;
      l_rval = errno;
      perror("application");
      goto exit;
    }
    m_boardVersion[0] = l_pData->at(13);
    m_boardVersion[1] = l_pData->at(12);
    m_boardVersion[2] = l_pData->at(10);
    m_boardVersion[3] = l_pData->at(9);
    cout << "Firmware version " << static_cast<uint16_t>(m_boardVersion[0])
         << "." << static_cast<uint16_t>(m_boardVersion[1]) << "."
         << static_cast<uint16_t>(m_boardVersion[2]) << "."
         << static_cast<uint16_t>(m_boardVersion[3]) << "." << endl;
    m_command[COMM_ID_READ_FIRMWARE_VERSION].setSuccessState();
    break;
  case COMM_ID_START_NO_PARAMS:
    cout << "COMM_ID_START_NO_PARAMS answer" << endl;
    m_scanSettings.m_shiftX = l_pData->at(9);
    cout << "ShiftX=" << static_cast<uint16_t>(m_scanSettings.m_shiftX) << endl;
    m_scanSettings.m_shiftY = l_pData->at(10);
    cout << "ShiftY=" << static_cast<uint16_t>(m_scanSettings.m_shiftY) << endl;
    m_scanSettings.m_sizeX = l_pData->at(11);
    cout << "SizeX=" << static_cast<uint16_t>(m_scanSettings.m_sizeX) << endl;
    m_scanSettings.m_sizeY = l_pData->at(12);
    cout << "SizeY=" << static_cast<uint16_t>(m_scanSettings.m_sizeY) << endl;
    m_scanSettings.m_samplesNumber = l_pData->at(13);
    cout << "SamplesNumber="
         << static_cast<uint16_t>(m_scanSettings.m_samplesNumber) << endl;
    m_scanSettings.m_framerate = (static_cast<uint16_t>(l_pData->at(15) << 8)) |
                                 (static_cast<uint16_t>(l_pData->at(14)));
    cout << "Framerate=" << static_cast<uint16_t>(m_scanSettings.m_framerate)
         << endl;

    m_scanSettings.m_adcSampleDelay =
        (static_cast<uint16_t>(l_pData->at(18) << 8)) |
        (static_cast<uint16_t>(l_pData->at(17)));
    cout << "AdcDelay="
         << static_cast<uint16_t>(m_scanSettings.m_adcSampleDelay) << endl;

    m_scanSettings.m_referenceVoltage =
        (static_cast<uint16_t>(l_pData->at(21) << 8)) |
        (static_cast<uint16_t>(l_pData->at(20)));
    cout << "ReferenceVoltage="
         << static_cast<uint16_t>(m_scanSettings.m_referenceVoltage) << endl;

    m_scanSettings.m_filterType = static_cast<uint16_t>(l_pData->at(22));

    m_scanTimestamp = (static_cast<uint16_t>(l_pData->at(24))) << 0;
    m_scanTimestamp |= (static_cast<uint16_t>(l_pData->at(25))) << 8;
    m_scanTimestamp |= (static_cast<uint16_t>(l_pData->at(27))) << 16;
    m_scanTimestamp |= (static_cast<uint16_t>(l_pData->at(28))) << 24;
    cout << "Timestamp=" << static_cast<uint32_t>(m_scanTimestamp) << endl;

    m_bitResolution = l_pData->at(30);
    cout << "BitResolution=" << static_cast<uint16_t>(m_bitResolution) << endl;

    m_boardVersion[3] = l_pData->at(32);
    m_boardVersion[2] = l_pData->at(33);
    m_boardVersion[1] = l_pData->at(35);
    m_boardVersion[0] = l_pData->at(36);
    cout << "Firmware version " << static_cast<uint32_t>(m_boardVersion[0])
         << "." << static_cast<uint32_t>(m_boardVersion[1]) << "."
         << static_cast<uint32_t>(m_boardVersion[2]) << "."
         << static_cast<uint32_t>(m_boardVersion[3]) << "." << endl;

    m_requestStatus = l_pData->at(37);
    cout << "RequestStatus=" << static_cast<uint16_t>(m_requestStatus) << endl;

    clearFrameBuffer();

    // Check if board settings corresponds to JSON
    if (m_jsonPresent) {
      if ((m_jsonTable.m_xSize > m_scanSettings.m_sizeX) ||
          (m_jsonTable.m_ySize > m_scanSettings.m_sizeY)) {
        cout << "Board settings and JSON settings mismatch. Stop scan" << endl;
        m_scanSettings.m_sizeX = m_jsonTable.m_xSize;
        m_scanSettings.m_sizeY = m_jsonTable.m_ySize;
        m_command[COMM_ID_WRITE_CONFIG].setExecuteState();
      }
    }

    m_scanState = ScanState::SCANNING;
    m_command[COMM_ID_START_NO_PARAMS].setSuccessState();
    break;
  case COMM_ID_RESET_JSON:
    m_command[COMM_ID_RESET_JSON].setSuccessState();
    break;
  case COMM_ID_WRITE_JSON:
    m_command[COMM_ID_WRITE_JSON].setSuccessState();
    break;
  case COMM_ID_APPLY_JSON:
    m_command[COMM_ID_APPLY_JSON].setSuccessState();
    break;
  case COMM_ID_GET_MATRIX_NAME:
    m_command[COMM_ID_GET_MATRIX_NAME].setSuccessState();
    break;
  case COMM_ID_WRITE_WIFI_CONF:
    m_command[COMM_ID_WRITE_WIFI_CONF].setSuccessState();
    break;
  case COMM_ID_READ_WIFI_CONF:
    m_command[COMM_ID_READ_WIFI_CONF].setSuccessState();
    break;
  case COMM_ID_WRITE_FLTR_CONF:
    m_command[COMM_ID_WRITE_FLTR_CONF].setSuccessState();
    break;
  case COMM_ID_READ_FLTR_CONF:
    m_command[COMM_ID_READ_FLTR_CONF].setSuccessState();
    break;
  case COMM_ID_RESET_CALIBR:
    m_command[COMM_ID_RESET_CALIBR].setSuccessState();
    break;
  case COMM_ID_WRITE_CALIBR_REF:
    m_command[COMM_ID_WRITE_CALIBR_REF].setSuccessState();
    break;
  case COMM_ID_WRITE_CALIBR_TABLE:
    m_command[COMM_ID_WRITE_CALIBR_TABLE].setSuccessState();
    break;
  case COMM_ID_READ_CALIBR:
    m_command[COMM_ID_READ_CALIBR].setSuccessState();
    break;
  case COMM_ID_READ_CALIBR_TABLE:
    m_command[COMM_ID_READ_CALIBR_TABLE].setSuccessState();
    break;
  case COMM_ID_READ_CALIBR_COMPLETE:
    m_command[COMM_ID_READ_CALIBR_COMPLETE].setSuccessState();
    break;
  case COMM_ID_WRITE_ACTIVE_CONF:
    m_command[COMM_ID_WRITE_ACTIVE_CONF].setSuccessState();
    break;
  case COMM_ID_READ_ACTIVE_CONF:
    m_command[COMM_ID_READ_ACTIVE_CONF].setSuccessState();
    break;
  default:
    errno = EBADMSG;
    l_rval = errno;
    perror("application");
    goto exit;
  }

  // Remove parsed data
  for (size_t i = 0; i < (l_dataLength + 7); i++) {
    l_pData->pop_front();
  }

exit:
  return l_rval;
}

/*Create frame buffer for store frames*/
int Application::clearFrameBuffer() {
  int l_rval = 0;

  lock_guard<mutex> lock(m_pInstance->m_FrameBufferLock);

  if (m_scanSettings.m_sizeX * m_scanSettings.m_sizeY == 0) {
    l_rval = -1;
  }

  if (m_pFrameBuffer.size() != 0) {
    m_pFrameBuffer.clear();
  }

  return l_rval;
}

/*Parse frame and put in to buffer*/
int Application::frameParser(std::deque<uint8_t> *l_frameData) {
  int l_rval = 0;
  std::deque<uint8_t>::iterator l_pData;
  Frame *l_pFrame = new Frame();

  size_t l_dataLength = 0;
  size_t l_expectedDataLength = 0;
  size_t l_dataByteSize = 0;
  uint32_t l_bitmask = 0;

  uint32_t l_packageId = 0;
  uint32_t l_timestamp = 0;

  auto time = std::chrono::system_clock::now();
  srand(std::chrono::system_clock::to_time_t(time));

  if (m_debugMode) {
    cout << "COMM_ID_FRAME answer" << endl;
    cout << "In queue: " << m_pFrameBuffer.size() << endl;
  }

  if (m_pFrameBuffer.size() >= FRAME_BUFFER_SIZE) {
    goto exit;
  }

  l_dataLength = (static_cast<size_t>(l_frameData->at(5))) << 0;
  l_dataLength |= (static_cast<size_t>(l_frameData->at(6))) << 8;

  l_dataByteSize = (m_bitResolution + 4) / 8;

  for (uint8_t i = 0; i < m_bitResolution; ++i) {
    l_bitmask = l_bitmask << 1;
    l_bitmask |= 0x01;
  }

  l_expectedDataLength =
      m_scanSettings.m_sizeX * m_scanSettings.m_sizeY * l_dataByteSize + 20;

  if (l_dataLength != l_expectedDataLength) {
    l_rval = -1;
    goto exit;
  }

  l_pFrame->init(m_scanSettings.m_sizeX * m_scanSettings.m_sizeY);

  l_packageId = (static_cast<uint32_t>(l_frameData->at(10))) << 0;
  l_packageId |= (static_cast<uint32_t>(l_frameData->at(11))) << 8;
  l_packageId |= (static_cast<uint32_t>(l_frameData->at(13))) << 16;
  l_packageId |= (static_cast<uint32_t>(l_frameData->at(14))) << 24;

  l_timestamp = (static_cast<time_t>(l_frameData->at(16))) << 0;
  l_timestamp = (static_cast<time_t>(l_frameData->at(17))) << 8;
  l_timestamp = (static_cast<time_t>(l_frameData->at(19))) << 16;
  l_timestamp = (static_cast<time_t>(l_frameData->at(20))) << 24;

  l_pFrame->m_packageId = l_packageId;
  l_pFrame->m_timestamp = l_timestamp;

  l_pData = l_frameData->begin();

  l_pData += 27;

  for (size_t i = 0; i < (m_scanSettings.m_sizeX * m_scanSettings.m_sizeY);
       i++) {

    l_pFrame->m_pFrame[i] = 0;

    for (size_t j = 0; j < l_dataByteSize; j++) {
      l_pFrame->m_pFrame[i] |= (uint32_t)(*l_pData) << (8 * j);
      ++l_pData;
    }
    l_pFrame->m_pFrame[i] &= l_bitmask;
  }

  l_pFrame = jsonConvert(l_pFrame);

  {
    lock_guard<mutex> lock(m_FrameBufferLock);
    m_pFrameBuffer.push_back(*l_pFrame);
  }

exit:
  return l_rval;
}

Application *Application::m_pInstance = nullptr;
