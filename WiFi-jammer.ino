#include "RF24.h"
#include <SPI.h>
#include "esp_bt.h"
#include "esp_wifi.h"

void (*soft_reset)() = 0; //dirty hack :)
void (*jamming)();        //pointer to jam func (depending on the connected nRF24L01(+))

bool useHSPI = false;
bool useVSPI = false;

//char '\' in code breaking all art but in terminal it's normal
static const char* logo = "                                                                                                 \n\
 _____                              _____       _       _    _ _______ _     _                                              \n\
|  ___|                            |  _  |     ( )     | |  | (_)  ___(_)   (_)                                             \n\
| |__ _ __ ___  _ __   ___ _ __ ___| |/' |_ __ |/ ___  | |  | |_| |_   _     _  __ _ _ __ ___  _ __ ___   ___ _ __          \n\
|  __| '_ ` _ \\| '_ \\ / _ \\ '__/ __|  /| | '_ \\  / __| | |/\\| | |  _| | |   | |/ _` | '_ ` _ \\| '_ ` _ \\ / _ \\ '__| \n\
| |__| | | | | | |_) |  __/ |  \\__ \\ |_/ / | | | \\__ \\ \\  /\\  / | |   | |   | | (_| | | | | | | | | | | |  __/ |      \n\
\\____/_| |_| |_| .__/ \\___|_|  |___/\\___/|_| |_| |___/  \\/  \\/|_\\_|   |_|   | |\\__,_|_| |_| |_|_| |_| |_|\\___|_|    \n\
               | |                                                         _/ |                                             \n\
               |_|                                                        |__/                                              \n\
";

static const uint8_t channels[28] = { //there are 1 main frequency and 2 sub frequency 
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
  74
	//84,  //14
  //87         //so it's banned in my country and there is no reason to jam it
};

SPIClass* vspi = nullptr; //vspi 
SPIClass* hspi = nullptr; //hspi

RF24 radio(16, 15, 16000000);  //you can chose your pins
RF24 radio1(22, 21, 16000000); //you can chose your pins

__attribute__((optimize("unroll-loops")))
void wifi_bl_Jamming() { //works on bluetooth(but not always) and wifi more efficient then wifiJamming() uses VSPI & HSPI
  for (int i = 0; i < sizeof(channels) / 2; i++) {
    radio1.setChannel(channels[i]);
    radio.setChannel(channels[i * 2]);
  }
}

__attribute__((optimize("unroll-loops")))
void wifi_bl_Jamming_V() { //works on bluetooth(but not always) and wifi more efficient then wifiJamming() uses VSPI & HSPI
  for (int i = 0; i < sizeof(channels); i++) {
    radio1.setChannel(channels[i]);
  }
}

__attribute__((optimize("unroll-loops")))
void wifi_bl_Jamming_H() { //works on bluetooth(but not always) and wifi more efficient then wifiJamming() uses HSPI
  for (int i = 0; i < sizeof(channels); i++) {
    radio.setChannel(channels[i]);
  }
}

void init_error() {
  Serial.print("Couldn't init VSPI or HSPI !!!");
  delay(1000);
}

void setup() {
  Serial.begin(115200);
  Serial.print(logo);
  esp_bt_controller_deinit();
  esp_wifi_deinit();
  initHSPI();
  initVSPI();
  if      (!(useHSPI || useVSPI)) jamming = &init_error;
  else if (useHSPI && useVSPI)    jamming = &wifi_bl_Jamming;
  else if (useVSPI)               jamming = &wifi_bl_Jamming_V;
  else if (useHSPI)               jamming = &wifi_bl_Jamming_H;
}

void initVSPI() {
  vspi = new SPIClass(VSPI);
  vspi->begin();
  if (radio1.begin(vspi)) {
    useVSPI = true;
    Serial.println("VSPI Started !!!");
    radio1.stopListening();
    radio1.setRetries(0, 0);
    radio1.setPALevel(RF24_PA_MAX, true);
    radio1.setDataRate(RF24_2MBPS);
    radio1.setCRCLength(RF24_CRC_DISABLED);
    radio1.setAutoAck(false);
    radio1.startConstCarrier(RF24_PA_MAX, 5);
    radio1.printPrettyDetails();
  } else {
    Serial.println("VSPI couldn't start !!!");
  }
}

void initHSPI() {
  hspi = new SPIClass(HSPI);
  hspi->begin();
  if (radio.begin(hspi)) {
    useHSPI = true;
    Serial.println("HSPI Started !!!");
    radio.stopListening();
    radio.setRetries(0, 0);
    radio.setPALevel(RF24_PA_MAX, true);
    radio.setDataRate(RF24_2MBPS);
    radio.setCRCLength(RF24_CRC_DISABLED);
    radio.setAutoAck(false);
    radio.startConstCarrier(RF24_PA_MAX, 8);
    radio.printPrettyDetails();
  } else {
    Serial.println("HSPI couldn't start !!!");
  }
}

uint16_t timer = 0;

void loop() {
  if (timer >= 16000) [[unlikely]] {
    Serial.println("Resetting...");
    soft_reset();
  }
  timer++;
  jamming(); //or wifiJamming()
  delay(1);
}
