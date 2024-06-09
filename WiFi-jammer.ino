#include "RF24.h"
#include <SPI.h>
#include "esp_bt.h"
#include "esp_wifi.h"

uint16_t channels[28] = { //there are 1 main frequency and 2 sub frequency 
  9,
	12, //1
  14,
	17, //2
  20,
	22, //3
  24,
	27, //4
  30,
	32, //5
  34,
	37, //6
  40,
	42, //7
  44,
	47, //8
  50,
	52, //9
  54,
	57, //10
  60,
	62, //11
  64,
	67, //12
  70,
	72, //13
  74,
  75 //to not invoke ub in baseJamming()
	//84,  //14
  //87         //so it's banned in my country and there is no reason to jam it
};

SPIClass *sp = nullptr;
SPIClass *hp = nullptr;

RF24 radio(16, 15, 16000000);   //HSPI CAN SET SPI SPEED TO 16000000 BY DEFAULT ITS 10000000
RF24 radio1(22, 21, 16000000);  //VSPI CAN SET SPI SPEED TO 16000000 BY DEFAULT ITS 10000000

uint8_t flagHSPI = 0;   //HSPI// Flag variable to keep track of direction
uint8_t flagVSPI = 0;  //VSPI// Flag variable to keep track of direction

uint8_t chVSPI = 45;
uint8_t chHSPI = 45;

const int num_reps = 50;

const uint8_t num_channels = 126;

uint8_t values[num_channels];

uint8_t* active = nullptr;

void extendedJamming() { //works on wifi and bluetooth
  (flagHSPI == 0)?chHSPI+=2:chHSPI-=2;
  (flagVSPI == 0)?chVSPI+=4:chVSPI-=4;

  if (flagHSPI == 0 && chHSPI >= 84) flagHSPI = 1;
  if (flagHSPI != 0 && chHSPI <= 1) flagHSPI = 0;

  if (flagVSPI == 0 && chVSPI >= 84) flagVSPI = 1;
  if (flagVSPI != 0 && chVSPI <= 1 ) flagVSPI = 0;
  //Serial.printf("chs: %d : %d\n", chHSPI, chVSPI);
  radio.setChannel(chHSPI);
  radio1.setChannel(chVSPI);
}

void baseJamming() {
  for (int i = 0; i < sizeof(channels) / 2; i++) {
    radio1.setChannel(channels[i]);
    radio.setChannel(channels[i * 2]);
  }
}

void setup() {
  Serial.begin(115200);
  esp_bt_controller_deinit();
  esp_wifi_stop();
  esp_wifi_deinit();
  esp_wifi_disconnect();
  initHP();
  initSP();
}

void initSP() {
  sp = new SPIClass(VSPI);
  sp->begin();
  if (radio1.begin(sp)) {
    Serial.println("SP Started !!!");
    radio1.setAutoAck(false);
    radio1.stopListening();
    radio1.setRetries(0, 0);
    radio1.setPALevel(RF24_PA_MAX, true);
    radio1.setDataRate(RF24_2MBPS);
    radio1.setCRCLength(RF24_CRC_DISABLED);
    radio1.printPrettyDetails();
    radio1.startConstCarrier(RF24_PA_MAX, chVSPI);
  } else {
    Serial.println("SP couldn't start !!!");
  }
}

void initHP() {
  hp = new SPIClass(HSPI);
  hp->begin();
  if (radio.begin(hp)) {
    Serial.println("HP Started !!!");
    radio.setAutoAck(false);
    radio.stopListening();
    radio.setRetries(0, 0);
    radio.setPALevel(RF24_PA_MAX, true);
    radio.setDataRate(RF24_2MBPS);
    radio.setCRCLength(RF24_CRC_DISABLED);
    radio.printPrettyDetails();
    radio.startConstCarrier(RF24_PA_MAX, chHSPI);
  } else {
    Serial.println("HP couldn't start !!!");
  }
}

void loop() {
  baseJamming();
  delay(1);
}
