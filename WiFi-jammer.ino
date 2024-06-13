//project Dalet... DaL is beautiful
#include "RF24.h"
#include <SPI.h>
#include "esp_bt.h"
#include "esp_wifi.h"

void (*soft_reset)() = 0; //dirty hack :)
void (*work)();           //pointer to jam func (depending on the connected nRF24L01(+))
uint8_t num_sub_freq = 2; //thats mean number of frequencies between left main frequency and right main frequency like this (4,{4.1),5,(5.1},6) in braces 1 main and 2 sub frequencies and for 4 sub frequency is 4.1 like for 5
char vt = -1;
bool useHSPI = false;
bool useVSPI = false;

uint8_t noise[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

//char '\' in code breaking all art but in terminal it's normal
inline const char* logo = "                                                                                                 \n\
 _____                              _____       _       _    _ _______ _     _                                              \n\
|  ___|                            |  _  |     ( )     | |  | (_)  ___(_)   (_)                                             \n\
| |__ _ __ ___  _ __   ___ _ __ ___| |/' |_ __ |/ ___  | |  | |_| |_   _     _  __ _ _ __ ___  _ __ ___   ___ _ __          \n\
|  __| '_ ` _ \\| '_ \\ / _ \\ '__/ __|  /| | '_ \\  / __| | |/\\| | |  _| | |   | |/ _` | '_ ` _ \\| '_ ` _ \\ / _ \\ '__| \n\
| |__| | | | | | |_) |  __/ |  \\__ \\ |_/ / | | | \\__ \\ \\  /\\  / | |   | |   | | (_| | | | | | | | | | | |  __/ |      \n\
\\____/_| |_| |_| .__/ \\___|_|  |___/\\___/|_| |_| |___/  \\/  \\/|_\\_|   |_|   | |\\__,_|_| |_| |_|_| |_| |_|\\___|_|    \n\
               | |                                                         _/ |                                             \n\
               |_|                                                        |__/                                              \n\
";

static const uint8_t channels[] = { //there are 1 main frequency and 2 sub frequency 
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

SPIClass* vspi = nullptr; //VSPI
SPIClass* hspi = nullptr; //HSPI

RF24 radio(16, 15, 16000000);  //you can chose your pins
RF24 radio1(22, 21, 16000000); //you can chose your pins

__attribute__((optimize("unroll-loops")))
void wifi_Jamming() { //works on wifi uses VSPI & HSPI
  for (int i = 0; i < sizeof(channels) / 2; i++) {
    radio1.setChannel(channels[i]);
    //radio1.write(noise, sizeof(noise));
    radio.setChannel(channels[i * 2]);
    //radio.write(noise, sizeof(noise));
  }
}

__attribute__((optimize("unroll-loops")))
void wifi_Jamming_V() { //works on wifi uses VSPI
  for (int i = 0; i < sizeof(channels); i++) {
    radio1.setChannel(channels[i]);
  }
}

__attribute__((optimize("unroll-loops")))
void wifi_Jamming_H() { //works on wifi uses HSPI
  for (int i = 0; i < sizeof(channels); i++) {
    radio.setChannel(channels[i]);
  }
}

void init_error() {
  Serial.print("Can't init VSPI or HSPI !!!");
  delay(1000);
}

void setup() {
  Serial.begin(115200);
  Serial.print(logo);
  esp_bt_controller_deinit();
  esp_wifi_deinit();
  Serial.print("--------------------------------HSPI--------------------------------\n");
  initHSPI();
  Serial.print("--------------------------------HSPI--------------------------------\n\n");
  Serial.print("--------------------------------VSPI--------------------------------\n");
  initVSPI();
  Serial.print("--------------------------------VSPI--------------------------------\n");
  char num_ch[2];
  itoa((sizeof(channels)-1)/num_sub_freq, num_ch, 10);
  Serial.print("Jamming channels: 1-");
  Serial.println(num_ch);
  if      (!(useHSPI || useVSPI)) {work = &init_error;     Serial.println("Can't run VSPI or HSPI");}
  else if (useHSPI && useVSPI)    {work = &wifi_Jamming;   Serial.println("Using VSPI and HSPI");   }
  else if (useVSPI)               {work = &wifi_Jamming_V; Serial.println("Using VSPI");            }
  else if (useHSPI)               {work = &wifi_Jamming_H; Serial.println("Using HSPI");            }
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
    radio.startConstCarrier(RF24_PA_MAX, 2);
    radio.printPrettyDetails();
  } else {
    Serial.println("HSPI couldn't start !!!");
  }
}

__attribute__((optimize("unroll-loops")))
void loop() {
  // static uint8_t timer = 0;
  // if (timer >= 255) [[unlikely]] {
  //   Serial.println("Resetting...");
  //   ESP.restart();
  // }
  // timer++;
  for (uint8_t i = 0; i <= (uint8_t)vt>>1;i++) work();
  delay(1);
}
