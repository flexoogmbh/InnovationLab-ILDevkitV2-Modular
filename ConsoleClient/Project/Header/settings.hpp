#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#define VERSION0 1
#define VERSION1 0
#define VERSION2 1

// HTTP
#define DEFAULT_SERVER_PORT 5001
#define CLIENT_INPUT_BUFFER_SIZE 4096U
#define SERVER_TASK_PERIOD 1 // mS

// Application settings
#define FRAME_BUFFER_SIZE 3 // pages
#define APP_CYCLE_PERIOD  50 // uS
#define APP_DETECT_PERIOD 250 // uS

// JSON
#define MAX_JSON_NAME_LENGTH 100
#define DEFAULT_JSON_FILE "frame.json"//"65x20.json"

// Matrix settings
#define MAX_X 96U
#define MAX_Y 96U
#define RX_BUFFER_SIZE (MAX_Y * MAX_X) + 100

// UNIX socket settings
#define UNIX_SOCKET "/tmp/PassiveMatrix"
#define INCOMMING_BUFFER_SIZE 1024

#define DO_START "start"
#define DO_STOP "stop"

// USB-Serial settings
#define PATH_LENGTH 256
#define SERIAL_PATH "/dev/"
#define MAX_SERIAL_PORT_NAME_LENGTH 100
#define DEFAULT_SERIAL_PORT "PassiveMatrix"//"ttyUSB0"//

#define USB_PART_BUFFER_SIZE 1000

// Baudrates:
// B57600
// B115200
// B230400
// B460800
// B500000
// B576000
// B921600
// B1000000
// B1152000
// B1500000
// B2000000
// B2500000
// B3000000
// B3500000
// B4000000
//__MAX_BAUD
#define BAUD_RATE B1000000

// Ethernet-TCP settings
#define NETWORK_PORT 1000

#define BOARDNAME "IL-PassiveMatrix"
#define BROADCAST "Broadcast"

#endif // SETTINGS_HPP
