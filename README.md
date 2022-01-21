# Printed Sensor Development Kit

## How to program
1.	Install Arduino IDE: https://www.arduino.cc/en/software 
2.	Run the IDE
3.	Install Nano BLE board support (Menu Tools/Board/Board Manager). Admin rights required.

![image](https://user-images.githubusercontent.com/49637543/150492833-09b0e56d-710f-4a48-8068-f1ba66bba625.png)

4.	Open the *.ino sketch

![image](https://user-images.githubusercontent.com/49637543/150492902-9a591992-31cf-4091-93ea-ab77d5cc4517.png)

5.	Select the proper board (Menu Tools/Board/Arduino Mbed OS Nano Boards/Arduino Nano 33 BLE)
6.	Select the proper com port (Menu Tools/Port)

![image](https://user-images.githubusercontent.com/49637543/150492975-bdee0b52-a486-4090-85da-c6080c177a60.png)

7.	Click Upload. The Sketch will be compiled and uploaded into the board.
8.	If the upload was successful, you should see following messages in log:

![image](https://user-images.githubusercontent.com/49637543/150493004-e782d3fd-98be-495a-9174-1d44be5a9455.png)

9.	Now you can use the board with SensorMatrixLAB (https://www.innovationlab.de/de/leistungen/software-download/) or free unix console client https://github.com/InnovationLabGmbH/InnovationLab-ILDevkitV2-Modular/tree/main/ConsoleClient.

## How to connect with console client
Console application supports following parameters:

--help - run information about parameters

--debug - run with debug log

--json filename.json - json file for frame conversion, in case you have none-standart matrix. Default: frame.json

--port 5001 - TCP server port, access local host to get data in JSON format. Default: 5001

--serial portname - name of serial port. Default: PassiveMatrix


Example (run in Ubuntu terminal):
```
./Matrix --serial ttyACM0
```
