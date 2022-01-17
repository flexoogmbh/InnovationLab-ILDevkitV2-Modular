#include "Application.h"

#define RX_COMMAND_SIZE    512
#define TX_COMMAND_SIZE   4096

#define SPI_SPEED      1000000
#define MUX_DELAY           10

#define RED_LED_PIN	        22
#define GREEN_LED_PIN	    23
#define BLUE_LED_PIN	    24

#define ANALOG_ENABLE_PIN   A2

#define INTERNAL_DAC		A0

#define MUX_SD			   D10
#define MUX_CS			    A6

#define DEMUX_EN			D9
#define DEMUX_CS			D8
#define DEMUX_WR			D7

#define DEMUX_S0		    D6
#define DEMUX_S1			D2
#define DEMUX_S2			D3
#define DEMUX_S3			D4
#define DEMUX_S4			D5

#define ADC_CS	     	    A7

#define EXT_TRIGGER			A3

Application g_mainApplication;

Led* g_pLed = nullptr;
MCP47X6* g_pDac = nullptr;
MAX14661* g_pMuxDriveLines = nullptr;
admux::Mux* g_pDeMuxScanLines = nullptr;
Ads7049* g_pAdc = nullptr;

// the setup function runs once when you press reset or power the board
void setup() {
  SPI.begin();
  Wire.begin();
  //pinMode(INTERNAL_DAC, OUTPUT);

  g_pLed = new Led(GREEN_LED_PIN, RED_LED_PIN, BLUE_LED_PIN);

  g_pDac = new MCP47X6;

  g_pMuxDriveLines = new MAX14661(MUX_CS, MUX_SD, 2, SPI_SPEED, MUX_DELAY);

  g_pDeMuxScanLines = new admux::Mux(admux::Pinset(DEMUX_S0, DEMUX_S1, DEMUX_S2, DEMUX_S3, DEMUX_S4), DEMUX_EN, DEMUX_CS, DEMUX_WR);

  g_pAdc = new Ads7049(ADC_CS, SPI_SPEED);

  // put your setup code here, to run once:
  g_mainApplication.init(g_pLed, RX_COMMAND_SIZE, TX_COMMAND_SIZE, ANALOG_ENABLE_PIN, INTERNAL_DAC, EXT_TRIGGER, g_pDac, g_pMuxDriveLines, g_pDeMuxScanLines, g_pAdc);

}

// the loop function runs over and over again forever
void loop() {
	static uint16_t s_blinkCounter = 0;

	g_mainApplication.checkCommand();
	
	delay(1);
	if (s_blinkCounter == 500) {
		g_pLed->set(Color::GREEEN);
	}
	else if (s_blinkCounter == 600) {
		g_pLed->set(Color::OFF);
		s_blinkCounter = 0;
	}
	++s_blinkCounter;
	
}
