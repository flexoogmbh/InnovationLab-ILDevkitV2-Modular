#include "Application.h"
#include "modbuscrc.h"
#include "Serial.h"

#pragma optimize( "", off )

ScanSettings::ScanSettings()
{
  reset();
}

ScanSettings::~ScanSettings() {}

void ScanSettings::reset()
{
  m_MatrixShiftX = 0;
  m_MatrixShiftY = 0;
  m_xSize = 0;
  m_ySize = 0;
  m_samplesNumber = 0;
  m_frameRate = 0;
  m_adcDelay = 0;
  m_refferenceVoltage = 0;
  m_filterType = FILTER_NONE;
  m_movAvrWindowSize = 5;
  m_movAverCumulativeFiltrationCoef = 0.05;
  m_movAverWeightedWindowSize = 5;
  m_medianWindowSize = 5;
  m_kalmanErrMeasure = 0.05;
  m_kalmanMeasureSpeed = 40;
}

Command::Command()
{
  reset();
}

Command::~Command()
{
}

bool Command::reset()
{
  m_code = 0;
  m_pData = nullptr;
  m_dataSize = 0;

  return true;
}

Application::Application()
{
  m_scanning = false;
  m_doStop = false;

  m_pRxBuffer = nullptr;
  m_rxBufferSize = 0;
  m_rxIndex = 0;

  m_pTxBuffer = nullptr;
  m_txBufferSize = 0;
  m_txLength = 0;

  m_UnixTime = 0;

  m_analogEnablePin = -1;

  m_pDac = nullptr;
  m_pMuxDriveLines = nullptr;
  m_pDeMuxScanLines = nullptr;
  m_pAdc = nullptr;

  m_externalTriggerFlag = false;
}

Application::~Application()
{
  delete[] m_pRxBuffer;
  delete[] m_pTxBuffer;
}

bool Application::init(Led* l_pLed, uint32_t l_rxBufferSize, uint32_t l_txBufferSize, int16_t l_analogEnablePin, int16_t l_internalDacPin, int16_t l_externalTriggerPin, MCP47X6 *l_pDac, MAX14661 *l_pMuxDriveLines, admux::Mux *l_pDeMuxScanLines, Ads7049 *l_pAdc)
{
  m_rxBufferSize = l_rxBufferSize;
  m_pRxBuffer = new uint8_t[m_rxBufferSize];

  m_txBufferSize = l_txBufferSize;
  m_pTxBuffer = new uint8_t[m_txBufferSize];

  m_pLed = l_pLed;;

  m_analogEnablePin = l_analogEnablePin;
  
  pinMode(m_analogEnablePin, OUTPUT);

  m_internalDacPin = l_internalDacPin;
  
  m_externalTriggerPin = l_externalTriggerPin;
  pinMode(m_externalTriggerPin, INPUT);

  m_pDac = l_pDac;
  m_pMuxDriveLines = l_pMuxDriveLines;
  m_pDeMuxScanLines = l_pDeMuxScanLines;
  m_pAdc = l_pAdc;

  readSettings();

  m_pDac->begin();
  m_pMuxDriveLines->begin();
  m_pAdc->begin();

  Serial.begin(1000000, SERIAL_8N1);

  return true;
}

volatile bool Application::scan()
{
  bool l_result = true;
  uint32_t l_pointsNumber = m_scanSettings.m_xSize * m_scanSettings.m_ySize;
  DATA_TYPE *l_pADCSamples = nullptr;
  uint32_t l_scanCounter = 0;
  uint32_t l_packageId = 0;
  unsigned long l_timestamp = 0;
  bool l_blink = false;

  //Setup voltage
  setReferenceVoltage(m_scanSettings.m_refferenceVoltage);

  //Reset stop flag
  m_doStop = false;

  //Check if no zero dimensions in parameters
  if (l_pointsNumber == 0)
  {
    l_result = false;
    goto exit;
  }

  //Allocate memory for data buffer
  l_pADCSamples = new DATA_TYPE[l_pointsNumber];

  //Check if memory allocated
  if (l_pADCSamples == nullptr)
  {
    l_result = false;
    goto exit;
  }

  for (uint32_t i = 0; i < l_pointsNumber; i++) {
      l_pADCSamples[i] = 0;
  }

  //Enable power supply of analogue module
  digitalWrite(m_analogEnablePin, HIGH);
  //Delay for power supply start
  delayMicroseconds(10000);

  m_pMuxDriveLines->enable();
  m_pMuxDriveLines->reset();

  switch (m_scanSettings.m_filterType) {
      case FILTER_NONE:
          m_pFilter = Filter::ByPass::getInstance();
          m_filterWindowSize = 0;
          break;
      case FILTER_MOV_AVERAGE:
          m_pFilter = Filter::MovAverage::getInstance(m_scanSettings.m_xSize* m_scanSettings.m_ySize, m_scanSettings.m_movAvrWindowSize);
          m_filterWindowSize = m_scanSettings.m_movAvrWindowSize;
          break;
      case FILTER_MEDIAN:
          m_pFilter = Filter::MedianFilter::getInstance(m_scanSettings.m_xSize * m_scanSettings.m_ySize, m_scanSettings.m_medianWindowSize);
          m_filterWindowSize = m_scanSettings.m_medianWindowSize;
          break;
      case FILTER_MOV_AVERAGE_CUMULATIVE:
          m_pFilter = Filter::MovAverageCumulative::getInstance(m_scanSettings.m_xSize * m_scanSettings.m_ySize, ((float)m_scanSettings.m_movAverCumulativeFiltrationCoef / 100));
          m_filterWindowSize = 5;
          break;
      case FILTER_MOV_AVERAGE_WEIGHTED:
          m_pFilter = Filter::MovAverageWeighted::getInstance(m_scanSettings.m_xSize * m_scanSettings.m_ySize, m_scanSettings.m_movAverWeightedWindowSize);
          m_filterWindowSize = m_scanSettings.m_movAverWeightedWindowSize;
          break;
      case FILTER_KALMAN:
          m_pFilter = Filter::KalmanFilter::getInstance(m_scanSettings.m_xSize * m_scanSettings.m_ySize, ((float)m_scanSettings.m_kalmanMeasureSpeed / 100), (float) m_scanSettings.m_kalmanErrMeasure);
          m_filterWindowSize = 5;
          break;
      }

  l_timestamp = millis();
  //Scan loop
  while (m_scanning)
  {
    //Scan process indicator
    if (l_blink) {
        m_pLed->set(Color::BLUE);
        l_blink = false;
    }
    else {
        m_pLed->set(Color::RED);
        l_blink = true;
    }

    //Check if any command received
    checkCommand();

    if (m_doStop)
    {
      m_doStop = false;
      l_result = false;
      goto exit;
    }

    m_pMuxDriveLines->setAllToB();                 // Prepare buffer
    m_pMuxDriveLines->switchChnlA(m_scanSettings.m_MatrixShiftX); // Switch first channel to Gnd

    for (uint32_t col = 0; col < m_scanSettings.m_xSize; ++col)
    {
      doSample(&l_pADCSamples[m_scanSettings.m_ySize * col], m_scanSettings.m_MatrixShiftY, m_scanSettings.m_ySize);

      m_pMuxDriveLines->setAllToB(); // Prepare buffer
      m_pMuxDriveLines->nextChnlA(); // Shitf 1 to the next bit (selecting the next column)
    }

    //Filter data
    if (!m_pFilter->insertData(l_pADCSamples)) {
        l_result = false;
        goto exit;
    }

    if (l_scanCounter >= m_filterWindowSize) {
        if (!m_pFilter->getData(l_pADCSamples)) {
            l_result = false;
            goto exit;
        }
        
        sendFrame(l_pADCSamples, l_pointsNumber, l_packageId, (millis() - l_timestamp), m_UnixTime);
        ++l_packageId;
    }
    else {
        ++l_scanCounter;
    }
    
    if ((m_scanSettings.m_samplesNumber != 0) && (l_packageId >= m_scanSettings.m_samplesNumber)) {
        break;
    }
    
  }
exit:
  //Clean filter
  if (m_pFilter != nullptr) {
      m_pFilter->free();
      m_pFilter = nullptr;
  }
  //Cleanup memory
  delete[] l_pADCSamples;
  //Disable MUX
  m_pMuxDriveLines->disable();
  //Disable reference
  setReferenceVoltage(0);
  //Power off analog part
  digitalWrite(m_analogEnablePin, LOW);
  m_pLed->set(Color::OFF);
  return l_result;
}

bool Application::checkCommand()
{
  Command l_command;
  getCommand(l_command);

  if (l_command.m_code == COMM_ID_EMPTY) {
      if (checkExtTriggerStart()) {
          l_command.m_code = COMM_ID_START_CAN;
          delay(1);
      }
      else if (checkExtTriggerStop()) {
          l_command.m_code = COMM_ID_STOP_CAN;
          delay(1);
      }
  }

  switch (l_command.m_code)
  {
  case COMM_ID_EMPTY:

    break;
  case COMM_ID_START:
    m_scanSettings.m_samplesNumber = l_command.m_pData[4];
    m_scanSettings.m_frameRate = (((uint16_t)l_command.m_pData[6] << 8) | l_command.m_pData[5]);
    m_scanSettings.m_adcDelay = (((uint16_t)l_command.m_pData[9] << 8) | l_command.m_pData[8]);
    //First answer frame
    if (m_scanning == false)
    {
      pushHeaderToAnswer();
      pushToAnswer(0);
      pushToAnswer(COMM_ID_START);
      pushToAnswer(m_scanSettings.m_MatrixShiftX);
      pushToAnswer(m_scanSettings.m_MatrixShiftY);
      pushToAnswer(m_scanSettings.m_xSize);
      pushToAnswer(m_scanSettings.m_ySize);
      pushToAnswer(m_scanSettings.m_samplesNumber);
      pushToAnswer((uint8_t)m_scanSettings.m_frameRate);
      pushToAnswer((uint8_t)(m_scanSettings.m_frameRate >> 8));
      pushToAnswer(0);
      pushToAnswer((uint8_t)m_scanSettings.m_adcDelay);
      pushToAnswer((uint8_t)(m_scanSettings.m_adcDelay >> 8));
      pushToAnswer(0);
      pushToAnswer((uint8_t)m_scanSettings.m_refferenceVoltage);
      pushToAnswer((uint8_t)(m_scanSettings.m_refferenceVoltage >> 8));
      pushToAnswer(0); //Filter type
      pushToAnswer(0);
      pushToAnswer((uint8_t)(m_UnixTime & 0xff));
      pushToAnswer((uint8_t)((m_UnixTime >> 8) & 0xff));
      pushToAnswer(0);
      pushToAnswer((uint8_t)((m_UnixTime >> 16) & 0xff));
      pushToAnswer((uint8_t)((m_UnixTime >> 24) & 0xff));
      pushToAnswer(0);
      pushToAnswer(DATA_RESOLUTION);
      pushToAnswer(0);
      pushToAnswer((uint8_t)(VERSION & 0xff));
      pushToAnswer((uint8_t)((VERSION >> 8) & 0xff));
      pushToAnswer(0);
      pushToAnswer((uint8_t)((VERSION >> 16) & 0xff));
      pushToAnswer((uint8_t)((VERSION >> 24) & 0xff));
      pushToAnswer(SESSION_STARTED);
      sendAnswer();

      //Set scan progress flag
      m_scanning = true;
      // Start scanning
      if (scan())
      {
          pushHeaderToAnswer();
          pushToAnswer(0);
          pushToAnswer(COMM_ID_STOP);
          pushToAnswer(0);
          sendAnswer();
      }
      //Set scan progress flag
      m_scanning = false;
    }
    else
    {
      m_doStop = true;
      pushHeaderToAnswer();
      pushToAnswer(0);
      pushToAnswer(COMM_ID_START_NO_PARAMS);
      pushToAnswer(m_scanSettings.m_MatrixShiftX);
      pushToAnswer(m_scanSettings.m_MatrixShiftY);
      pushToAnswer(m_scanSettings.m_xSize);
      pushToAnswer(m_scanSettings.m_ySize);
      pushToAnswer(m_scanSettings.m_samplesNumber);
      pushToAnswer((uint8_t)m_scanSettings.m_frameRate);
      pushToAnswer((uint8_t)(m_scanSettings.m_frameRate >> 8));
      pushToAnswer(0);
      pushToAnswer((uint8_t)m_scanSettings.m_adcDelay);
      pushToAnswer((uint8_t)(m_scanSettings.m_adcDelay >> 8));
      pushToAnswer(0);
      pushToAnswer((uint8_t)m_scanSettings.m_refferenceVoltage);
      pushToAnswer((uint8_t)(m_scanSettings.m_refferenceVoltage >> 8));
      pushToAnswer(0); //Filter type
      pushToAnswer(0);
      pushToAnswer((uint8_t)(m_UnixTime & 0xff));
      pushToAnswer((uint8_t)((m_UnixTime >> 8) & 0xff));
      pushToAnswer(0);
      pushToAnswer((uint8_t)((m_UnixTime >> 16) & 0xff));
      pushToAnswer((uint8_t)((m_UnixTime >> 24) & 0xff));
      pushToAnswer(0);
      pushToAnswer(DATA_RESOLUTION);
      pushToAnswer(0);
      pushToAnswer((uint8_t)(VERSION & 0xff));
      pushToAnswer((uint8_t)((VERSION >> 8) & 0xff));
      pushToAnswer(0);
      pushToAnswer((uint8_t)((VERSION >> 16) & 0xff));
      pushToAnswer((uint8_t)((VERSION >> 24) & 0xff));
      pushToAnswer(SESSION_STOPED);
      sendAnswer();
    }
    break;
  case COMM_ID_STOP:
    m_scanning = false;
    pushHeaderToAnswer();
    pushToAnswer(0);
    pushToAnswer(COMM_ID_STOP);
    pushToAnswer(0);
    sendAnswer();
    break;
  case COMM_ID_START_CAN:
      pushHeaderToAnswer();
      pushToAnswer(0);
      pushToAnswer(COMM_ID_START_CAN);
      pushToAnswer(m_scanSettings.m_MatrixShiftX);
      pushToAnswer(m_scanSettings.m_MatrixShiftY);
      pushToAnswer(m_scanSettings.m_xSize);
      pushToAnswer(m_scanSettings.m_ySize);
      pushToAnswer(m_scanSettings.m_samplesNumber);
      pushToAnswer((uint8_t)m_scanSettings.m_frameRate);
      pushToAnswer((uint8_t)(m_scanSettings.m_frameRate >> 8));
      pushToAnswer(0);
      pushToAnswer((uint8_t)m_scanSettings.m_adcDelay);
      pushToAnswer((uint8_t)(m_scanSettings.m_adcDelay >> 8));
      pushToAnswer(0);
      pushToAnswer((uint8_t)m_scanSettings.m_refferenceVoltage);
      pushToAnswer((uint8_t)(m_scanSettings.m_refferenceVoltage >> 8));
      pushToAnswer(0); //Filter type
      pushToAnswer(0);
      pushToAnswer((uint8_t)(m_UnixTime & 0xff));
      pushToAnswer((uint8_t)((m_UnixTime >> 8) & 0xff));
      pushToAnswer(0);
      pushToAnswer((uint8_t)((m_UnixTime >> 16) & 0xff));
      pushToAnswer((uint8_t)((m_UnixTime >> 24) & 0xff));
      pushToAnswer(0);
      pushToAnswer(DATA_RESOLUTION);
      pushToAnswer(0);
      pushToAnswer((uint8_t)(VERSION & 0xff));
      pushToAnswer((uint8_t)((VERSION >> 8) & 0xff));
      pushToAnswer(0);
      pushToAnswer((uint8_t)((VERSION >> 16) & 0xff));
      pushToAnswer((uint8_t)((VERSION >> 24) & 0xff));
      pushToAnswer(SESSION_STARTED);
      sendAnswer();

      //Set scan progress flag
      m_scanning = true;
      // Start scanning
      if (scan())
      {
          pushHeaderToAnswer();
          pushToAnswer(0);
          pushToAnswer(COMM_ID_STOP_CAN);
          pushToAnswer(0);
          sendAnswer();
      }
      //Set scan progress flag
      m_scanning = false;
      break;
  case COMM_ID_FRAME:

    break;
  case COMM_ID_STOP_CAN:
      m_scanning = false;
      break;
  case COMM_ID_WRITE_CONFIG:
    readSettings();
    // Get received data

    m_scanSettings.m_MatrixShiftX = l_command.m_pData[0];

    m_scanSettings.m_MatrixShiftY = l_command.m_pData[1];

    m_scanSettings.m_xSize = l_command.m_pData[2];

    if (m_scanSettings.m_xSize == 0)
    {
      m_scanSettings.m_xSize = MAX_X;
    }

    m_scanSettings.m_ySize = l_command.m_pData[3];
    if (m_scanSettings.m_ySize == 0)
    {
      m_scanSettings.m_ySize = MAX_Y;
    }

    m_scanSettings.m_samplesNumber = l_command.m_pData[4];
    m_scanSettings.m_frameRate = (((uint16_t)l_command.m_pData[6] << 8) | l_command.m_pData[5]);
    m_scanSettings.m_adcDelay = (((uint16_t)l_command.m_pData[9] << 8) | l_command.m_pData[8]);

    //11-12 Offset voltage
    m_scanSettings.m_refferenceVoltage = (((uint16_t)l_command.m_pData[14] << 8) | l_command.m_pData[13]);
    if (m_scanSettings.m_refferenceVoltage == 0)
    {
      m_scanSettings.m_refferenceVoltage = 1;
    }

    //15 Filter type
    m_scanSettings.m_filterType = l_command.m_pData[15];

    if (writeSettings())
    {
      pushHeaderToAnswer();
      pushToAnswer(0);
      pushToAnswer(COMM_ID_WRITE_CONFIG);
      sendAnswer();
    }
    break;
  case COMM_ID_READ_CONFIG:
    pushHeaderToAnswer();

    pushToAnswer(0);
    pushToAnswer(COMM_ID_READ_CONFIG);

    pushToAnswer(m_scanSettings.m_MatrixShiftX);              //X shift
    pushToAnswer(m_scanSettings.m_MatrixShiftY);              //Y shift
    pushToAnswer(m_scanSettings.m_xSize);                     //X size
    pushToAnswer(m_scanSettings.m_ySize);                     //Y size
    pushToAnswer(m_scanSettings.m_samplesNumber);             //Number of samples
    pushToAnswer((uint8_t)m_scanSettings.m_frameRate);        //Framerate (low byte)
    pushToAnswer((uint8_t)(m_scanSettings.m_frameRate >> 8)); //Framerate (high byte)
    pushToAnswer(0);
    pushToAnswer((uint8_t)m_scanSettings.m_adcDelay);        //ADC delay (low byte)
    pushToAnswer((uint8_t)(m_scanSettings.m_adcDelay >> 8)); //ADC delay (high byte)
    pushToAnswer(0);
    pushToAnswer(0);                                                  //Offset voltage (low byte)
    pushToAnswer(0);                                                  //Offset voltage (high byte)
    pushToAnswer((uint8_t)m_scanSettings.m_refferenceVoltage);        //Reference voltage (low byte)
    pushToAnswer((uint8_t)(m_scanSettings.m_refferenceVoltage >> 8)); //Reference voltage (high byte)

    pushToAnswer((uint8_t)(m_scanSettings.m_filterType)); //Filter type

    sendAnswer();
    break;
  case COMM_ID_READ_FIRMWARE_VERSION:
    pushHeaderToAnswer();

    pushToAnswer(0);
    pushToAnswer(COMM_ID_READ_FIRMWARE_VERSION);

    pushToAnswer((uint8_t)(VERSION & 0xff));
    pushToAnswer((uint8_t)((VERSION >> 8) & 0xff));
    pushToAnswer(0);
    pushToAnswer((uint8_t)((VERSION >> 16) & 0xff));
    pushToAnswer((uint8_t)((VERSION >> 24) & 0xff));

    sendAnswer();
    break;
  case COMM_ID_START_NO_PARAMS:
    //First answer frame
    if (m_scanning == false)
    {
      pushHeaderToAnswer();
      pushToAnswer(0);
      pushToAnswer(COMM_ID_START_NO_PARAMS);
      pushToAnswer(m_scanSettings.m_MatrixShiftX);
      pushToAnswer(m_scanSettings.m_MatrixShiftY);
      pushToAnswer(m_scanSettings.m_xSize);
      pushToAnswer(m_scanSettings.m_ySize);
      pushToAnswer(m_scanSettings.m_samplesNumber);
      pushToAnswer((uint8_t)m_scanSettings.m_frameRate);
      pushToAnswer((uint8_t)(m_scanSettings.m_frameRate >> 8));
      pushToAnswer(0);
      pushToAnswer((uint8_t)m_scanSettings.m_adcDelay);
      pushToAnswer((uint8_t)(m_scanSettings.m_adcDelay >> 8));
      pushToAnswer(0);
      pushToAnswer((uint8_t)m_scanSettings.m_refferenceVoltage);
      pushToAnswer((uint8_t)(m_scanSettings.m_refferenceVoltage >> 8));
      pushToAnswer(0); //Filter type
      pushToAnswer(0);
      pushToAnswer((uint8_t)(m_UnixTime & 0xff));
      pushToAnswer((uint8_t)((m_UnixTime >> 8) & 0xff));
      pushToAnswer(0);
      pushToAnswer((uint8_t)((m_UnixTime >> 16) & 0xff));
      pushToAnswer((uint8_t)((m_UnixTime >> 24) & 0xff));
      pushToAnswer(0);
      pushToAnswer(DATA_RESOLUTION);
      pushToAnswer(0);
      pushToAnswer((uint8_t)(VERSION & 0xff));
      pushToAnswer((uint8_t)((VERSION >> 8) & 0xff));
      pushToAnswer(0);
      pushToAnswer((uint8_t)((VERSION >> 16) & 0xff));
      pushToAnswer((uint8_t)((VERSION >> 24) & 0xff));
      pushToAnswer(SESSION_STARTED);
      sendAnswer();

      //Set scan progress flag
      m_scanning = true;
      // Start scanning
      if (scan())
      {
        pushHeaderToAnswer();
        pushToAnswer(0);
        pushToAnswer(COMM_ID_STOP);
        pushToAnswer(0);
        sendAnswer();
      }
      //Set scan progress flag
      m_scanning = false;
    }
    else
    {
      m_doStop = true;

      pushHeaderToAnswer();
      pushToAnswer(0);
      pushToAnswer(COMM_ID_START_NO_PARAMS);
      pushToAnswer(m_scanSettings.m_MatrixShiftX);
      pushToAnswer(m_scanSettings.m_MatrixShiftY);
      pushToAnswer(m_scanSettings.m_xSize);
      pushToAnswer(m_scanSettings.m_ySize);
      pushToAnswer(m_scanSettings.m_samplesNumber);
      pushToAnswer((uint8_t)m_scanSettings.m_frameRate);
      pushToAnswer((uint8_t)(m_scanSettings.m_frameRate >> 8));
      pushToAnswer(0);
      pushToAnswer((uint8_t)m_scanSettings.m_adcDelay);
      pushToAnswer((uint8_t)(m_scanSettings.m_adcDelay >> 8));
      pushToAnswer(0);
      pushToAnswer((uint8_t)m_scanSettings.m_refferenceVoltage);
      pushToAnswer((uint8_t)(m_scanSettings.m_refferenceVoltage >> 8));
      pushToAnswer(0); //Filter type
      pushToAnswer(0);
      pushToAnswer((uint8_t)(m_UnixTime & 0xff));
      pushToAnswer((uint8_t)((m_UnixTime >> 8) & 0xff));
      pushToAnswer(0);
      pushToAnswer((uint8_t)((m_UnixTime >> 16) & 0xff));
      pushToAnswer((uint8_t)((m_UnixTime >> 24) & 0xff));
      pushToAnswer(0);
      pushToAnswer(DATA_RESOLUTION);
      pushToAnswer(0);
      pushToAnswer((uint8_t)(VERSION & 0xff));
      pushToAnswer((uint8_t)((VERSION >> 8) & 0xff));
      pushToAnswer(0);
      pushToAnswer((uint8_t)((VERSION >> 16) & 0xff));
      pushToAnswer((uint8_t)((VERSION >> 24) & 0xff));
      pushToAnswer(SESSION_STOPED);
      sendAnswer();
    }
    break;
  case COMM_ID_WRITE_FLTR_CONF:
      readSettings();

      m_scanSettings.m_movAvrWindowSize = l_command.m_pData[0];
      m_scanSettings.m_movAverCumulativeFiltrationCoef = l_command.m_pData[2];
      m_scanSettings.m_movAverWeightedWindowSize = l_command.m_pData[4];
      m_scanSettings.m_medianWindowSize = l_command.m_pData[6];
      m_scanSettings.m_kalmanMeasureSpeed = l_command.m_pData[8];
      m_scanSettings.m_kalmanErrMeasure = l_command.m_pData[9];

      if (writeSettings())
      {
          pushHeaderToAnswer();
          pushToAnswer(0);
          pushToAnswer(COMM_ID_WRITE_FLTR_CONF);
          sendAnswer();
      }
    break;
  case COMM_ID_READ_FLTR_CONF:
      readSettings();
      pushHeaderToAnswer();
      pushToAnswer(0);
      pushToAnswer(COMM_ID_READ_FLTR_CONF);
      pushToAnswer(0);
      pushToAnswer(m_scanSettings.m_movAvrWindowSize);
      pushToAnswer(0);
      pushToAnswer((uint8_t) (m_scanSettings.m_movAverCumulativeFiltrationCoef));
      pushToAnswer(0);
      pushToAnswer(m_scanSettings.m_movAverWeightedWindowSize);
      pushToAnswer(0);
      pushToAnswer(m_scanSettings.m_medianWindowSize);
      pushToAnswer(0);
      pushToAnswer((uint8_t)(m_scanSettings.m_kalmanMeasureSpeed));
      pushToAnswer((uint8_t) m_scanSettings.m_kalmanErrMeasure);
      pushToAnswer(0);
      sendAnswerCRC();
    break;
  default:

    break;
  }

  return true;
}

bool Application::getCommand(Command &l_command)
{
  bool l_result = true;
  uint8_t *l_pRxData = nullptr;
  int l_dataSize = Serial.available();
  uint16_t l_crc = 0;
  l_command.reset();

  if (l_dataSize == 0)
  {
    l_result = false;
    goto exit;
  }

  //Create temp buffer for reading
  l_pRxData = new uint8_t[l_dataSize];
  //Check is buffer created
  if (l_pRxData == nullptr)
  {
    l_result = false;
    goto exit;
  }

  l_dataSize = Serial.readBytes(l_pRxData, l_dataSize);

  for (uint16_t i = 0; i < l_dataSize; i++)
  {
    m_pRxBuffer[m_rxIndex] = l_pRxData[i];
    ++m_rxIndex;
  }

  if (m_rxIndex < 9)
  {
    l_result = false;
    goto exit;
  }

  if (!checkPreamble(m_pRxBuffer))
  {
    l_result = false;
    goto exit;
  }

  if (m_rxIndex >= 9 && m_pRxBuffer[8] == COMM_ID_STOP)
  {
    l_command.m_dataSize = (((uint16_t)m_pRxBuffer[6] << 8) | (uint16_t)m_pRxBuffer[5]);
    if (l_command.m_dataSize != 2)
    {
      l_result = false;
      goto exit;
    }

    l_command.m_code = COMM_ID_STOP;
  }
  else if (m_rxIndex >= 19 && m_pRxBuffer[8] == COMM_ID_START)
  {
    l_command.m_dataSize = (((uint16_t)m_pRxBuffer[6] << 8) | (uint16_t)m_pRxBuffer[5]);
    if (l_command.m_dataSize != 12)
    {
      l_result = false;
      goto exit;
    }

    l_command.m_code = COMM_ID_START;
    l_command.m_pData = &m_pRxBuffer[9];

    m_rxIndex = 0;
  }
  else if (m_rxIndex >= 25 && m_pRxBuffer[8] == COMM_ID_WRITE_CONFIG)
  {
    l_command.m_dataSize = (((uint16_t)m_pRxBuffer[6] << 8) | (uint16_t)m_pRxBuffer[5]);
    if (l_command.m_dataSize != 0x12)
    {
      l_result = false;
      goto exit;
    }

    l_command.m_code = COMM_ID_WRITE_CONFIG;
    l_command.m_pData = &m_pRxBuffer[9];
  }
  else if (m_rxIndex >= 9 && m_pRxBuffer[8] == COMM_ID_READ_CONFIG)
  {
    l_command.m_dataSize = (((uint16_t)m_pRxBuffer[6] << 8) | (uint16_t)m_pRxBuffer[5]);
    if (l_command.m_dataSize != 2)
    {
      l_result = false;
      goto exit;
    }

    l_command.m_code = COMM_ID_READ_CONFIG;
  }
  else if (m_rxIndex >= 9 && m_pRxBuffer[8] == COMM_ID_READ_FIRMWARE_VERSION)
  {
    l_command.m_dataSize = (((uint16_t)m_pRxBuffer[6] << 8) | (uint16_t)m_pRxBuffer[5]);
    if (l_command.m_dataSize != 2)
    {
      l_result = false;
      goto exit;
    }

    l_command.m_code = COMM_ID_READ_FIRMWARE_VERSION;
  }
  else if (m_rxIndex >= 9 && m_pRxBuffer[8] == COMM_ID_START_NO_PARAMS)
  {
    l_command.m_dataSize = (((uint16_t)m_pRxBuffer[6] << 8) | (uint16_t)m_pRxBuffer[5]);
    if (l_command.m_dataSize != 2)
    {
      l_result = false;
      goto exit;
    }
    l_command.m_code = COMM_ID_START_NO_PARAMS;
  }
  else if (m_rxIndex >= 22 && m_pRxBuffer[8] == COMM_ID_WRITE_FLTR_CONF) {

      l_command.m_dataSize = (((uint16_t)m_pRxBuffer[6] << 8) | (uint16_t)m_pRxBuffer[5]);

      if (l_command.m_dataSize != 16) {
          l_result = false;
          goto exit;
      }

      l_crc = mbCrc(&m_pRxBuffer[7], (l_command.m_dataSize - 2));

      if (l_crc != ((((uint16_t)m_pRxBuffer[21]) << 8) | ((uint16_t)m_pRxBuffer[22]))) {
          l_result = false;
          goto exit;
      }

      l_command.m_code = COMM_ID_WRITE_FLTR_CONF;
      l_command.m_pData = &m_pRxBuffer[10];

  }
  else if (m_rxIndex >= 9 && m_pRxBuffer[8] == COMM_ID_READ_FLTR_CONF) {
      l_command.m_dataSize = (((uint16_t)m_pRxBuffer[6] << 8) | (uint16_t)m_pRxBuffer[5]);
      if (l_command.m_dataSize != 2) {
          l_result = false;
          goto exit;
      }

      l_command.m_code = COMM_ID_READ_FLTR_CONF;
  }

exit:
  delete[] l_pRxData;

  if ((!l_result) ||                       //Command fail
      (l_command.m_code != COMM_ID_EMPTY)) //Command OK
  {
    m_rxIndex = 0;
  }

  return l_result;
}

bool Application::writeSettings()
{
  ret_code_t l_result = myFlashPrefs.writePrefs(&m_scanSettings, sizeof(m_scanSettings));

  return (l_result == FDS_SUCCESS);
}

bool Application::readSettings()
{
  ret_code_t l_result = myFlashPrefs.readPrefs(&m_scanSettings, sizeof(m_scanSettings));

  if (m_scanSettings.m_movAvrWindowSize >= MOV_AVERAGE_MAX_WINDOW_SIZE) {
      m_scanSettings.m_movAvrWindowSize = MOV_AVERAGE_MAX_WINDOW_SIZE;
  }

  if (m_scanSettings.m_movAverCumulativeFiltrationCoef >= MOV_AVERAGE_CUMMULATIVE_MAX_COEF) {
      m_scanSettings.m_movAverCumulativeFiltrationCoef = MOV_AVERAGE_CUMMULATIVE_MAX_COEF;
  }

  if (m_scanSettings.m_movAverWeightedWindowSize >= MOV_AVERAGE_WEIGHTED_MAX_WINDOW_SIZE) {
      m_scanSettings.m_movAverWeightedWindowSize = MOV_AVERAGE_WEIGHTED_MAX_WINDOW_SIZE;
  }

  if (m_scanSettings.m_medianWindowSize >= MEDIAN_MAX_WINDOW_SIZE) {
      m_scanSettings.m_medianWindowSize = MEDIAN_MAX_WINDOW_SIZE;
  }

  if (m_scanSettings.m_kalmanErrMeasure >= KALMAN_MAX_ERROR) {
      m_scanSettings.m_kalmanErrMeasure = KALMAN_MAX_ERROR;
  }

  if (m_scanSettings.m_kalmanMeasureSpeed >= KALMAN_MAX_SPEED) {
      m_scanSettings.m_kalmanMeasureSpeed = KALMAN_MAX_SPEED;
  }

  return (l_result == FDS_SUCCESS);
}

bool Application::checkExtTriggerStart() 
{
    if (!m_externalTriggerFlag && (digitalRead(m_externalTriggerPin) == LOW)) {
        m_externalTriggerFlag = true;
        return true;
    }
    else {
        return false;
    }
}

bool Application::checkExtTriggerStop()
{
    if (m_externalTriggerFlag && (digitalRead(m_externalTriggerPin) == HIGH)) {
        m_externalTriggerFlag = false;
        return true;
    }
    else {
        return false;
    }
}

bool Application::checkPreamble(uint8_t *l_pData)
{
  if ((l_pData[0] == 0xFF) &&
      (l_pData[1] == 0xFF) &&
      (l_pData[2] == 0xFF) &&
      (l_pData[3] == 0xFF) &&
      (l_pData[4] == 0x00))
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool Application::resetAnswer()
{
  memset(m_pTxBuffer, 0, m_txBufferSize);
  m_txLength = 0;

  return true;
}

bool Application::pushHeaderToAnswer()
{
  resetAnswer();

  pushToAnswer(0xFF); //Preamble
  pushToAnswer(0xFF); //Preamble
  pushToAnswer(0xFF); //Preamble
  pushToAnswer(0xFF); //Preamble

  pushToAnswer(0); //Divider

  pushToAnswer(0); //Data lenght low byte (sets while sending)
  pushToAnswer(0); //Data lenght low byte (sets while sending)

  return true;
}

bool Application::pushToAnswer(uint8_t l_data)
{
  if (m_txLength >= m_txBufferSize)
  {
    return false;
  }

  m_pTxBuffer[m_txLength] = l_data;
  ++m_txLength;

  return true;
}

bool Application::sendAnswer()
{
  size_t l_txLength = 0;

  m_pTxBuffer[5] = (uint8_t)(m_txLength - 7);        //Data length
  m_pTxBuffer[6] = (uint8_t)((m_txLength - 7) >> 8); //Data length

  l_txLength = Serial.write(m_pTxBuffer, m_txLength);

  if (l_txLength == m_txLength)
  {
    resetAnswer();
    return true;
  }
  else
  {
    return false;
  }
}

bool Application::sendAnswerCRC()
{
    size_t l_txLength = 0;
    uint16_t l_crc = 0;

    l_crc = mbCrc(&m_pTxBuffer[7], (m_txLength - 7));

    pushToAnswer((uint8_t)((l_crc >> 8) & 0xFF));
    pushToAnswer((uint8_t)((l_crc >> 0) & 0xFF));

    m_pTxBuffer[5] = (uint8_t)(m_txLength - 7);        //Data length
    m_pTxBuffer[6] = (uint8_t)((m_txLength - 7) >> 8); //Data length

    l_txLength = Serial.write(m_pTxBuffer, m_txLength);

    if (l_txLength == m_txLength)
    {
        resetAnswer();
        return true;
    }
    else
    {
        return false;
    }
}

bool Application::setReferenceVoltage(uint16_t l_dacVoltage)
{
  bool l_result = true;
  
  /*Set external DAC*/

  //MCP47X6_VREF_VDD
  //MCP47X6_VREF_VREFPIN
  //MCP47X6_VREF_VREFPIN_BUFFERED
  uint8_t l_vref = MCP47X6_VREF_VREFPIN_BUFFERED;

  //MCP47X6_GAIN_1X
  //MCP47X6_GAIN_2X
  uint8_t l_gain = MCP47X6_GAIN_1X;

  //1024 maximum steps
  // REG = (Vout * 4096) / (Vref * GAIN)
  uint16_t l_level = (l_dacVoltage * 4096) / (330 * 1);

  m_pDac->begin(0);
  m_pDac->setVReference(l_vref);
  m_pDac->setGain(l_gain);
  m_pDac->powerDown(MCP47X6_PWRDN_100K);
  m_pDac->saveSettings();
  m_pDac->setOutputLevel(l_level);

  return l_result;
}

bool Application::doSample(uint16_t *buffer, uint32_t rowStart, uint32_t rowCount)
{
  bool l_result = true;

  for (uint32_t row = rowStart; row < rowCount; row++) {
      // Select MUX address
      m_pDeMuxScanLines->channel(row);

      delayMicroseconds(m_scanSettings.m_adcDelay);
      // Conversion
      m_pAdc->getValue(&buffer[row]);
  }

  return l_result;
}

bool Application::sendFrame(DATA_TYPE *l_pData, uint32_t l_dataLength, uint32_t l_packageId, uint32_t l_timestamp, uint32_t l_unixTime)
{
  pushHeaderToAnswer();
  pushToAnswer(0);
  pushToAnswer(COMM_ID_FRAME);

  pushToAnswer(0);

  pushToAnswer((uint8_t)(l_packageId & 0xff));
  pushToAnswer((uint8_t)((l_packageId >> 8) & 0xff));
  pushToAnswer(0x00);
  pushToAnswer((uint8_t)((l_packageId >> 16) & 0xff));
  pushToAnswer((uint8_t)((l_packageId >> 24) & 0xff));
  pushToAnswer(0x00);
  pushToAnswer((uint8_t)(l_timestamp & 0xff));
  pushToAnswer((uint8_t)((l_timestamp >> 8) & 0xff));
  pushToAnswer(0x00);
  pushToAnswer((uint8_t)((l_timestamp >> 16) & 0xff));
  pushToAnswer((uint8_t)((l_timestamp >> 24) & 0xff));
  pushToAnswer(0x00);
  pushToAnswer((uint8_t)(l_unixTime & 0xff));
  pushToAnswer((uint8_t)((l_unixTime >> 8) & 0xff));
  pushToAnswer(0x00);
  pushToAnswer((uint8_t)((l_unixTime >> 16) & 0xff));
  pushToAnswer((uint8_t)((l_unixTime >> 24) & 0xff));

  for (uint16_t index = 0; index < l_dataLength; index++)
  {
    for (uint16_t j = 0; j < sizeof(DATA_TYPE); j++)
    {
      pushToAnswer((uint8_t)((l_pData[index] >> (8 * j)) & 0xff));
    }
  }

  return sendAnswer();
}
