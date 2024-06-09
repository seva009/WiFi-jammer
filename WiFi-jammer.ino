#include <LittleFS.h>
#include <EEPROM.h>
#include "RF24.h"
#include <ezButton.h>
#include "esp_bt.h"
#include "esp_wifi.h"

SPIClass *sp = nullptr;

static const uint8_t num_channels = 128;
static uint8_t values[num_channels];
static uint32_t end_vals[num_channels];
static const uint8_t active_ch = 0;

RF24 radio(22, 21, 16000000);  /// CAN SET SPI SPEED TO 16000000 BY DEFAULT ITS 10000000

void full_scan() {
  memset(end_vals, 0x01, num_channels);
  if (!LittleFS.begin()) {
    LittleFS.format();
    Serial.println("Can't mount ffat!");
    ESP.restart();
  }
  sp = new SPIClass(VSPI);
  sp->begin();
  radio.begin(sp);
  radio.setAutoAck(false);
  radio.startListening();
  radio.stopListening();
  radio.printDetails();

  int i = 0;
  while (i < num_channels) {
    Serial.print(i >> 4, HEX);
    ++i;
  }
  Serial.println();
  i = 0;
  while (i < num_channels) {
    Serial.print(i & 0xf, HEX);
    ++i;
  }
  Serial.println();

  const int num_reps = 100;
  for (uint8_t z = 0; z < 20; z++) {
  // Clear measurement values
    memset(values, 0, sizeof(values));

    // Scan all channels num_reps times
    int rep_counter = num_reps;
    while (rep_counter--) {
      int i = num_channels;
      while (i--) {
        // Select this channel
        radio.setChannel(i);

        // Listen for a little
        radio.startListening();
        delayMicroseconds(128);
        radio.stopListening();

        // Did we get a carrier?
        if (radio.testCarrier()) {
          ++values[i];
        }
        delay(1);
      }
      delay(1);
    }


    // Print out channel measurements, clamped to a single hex digit
    int i = 0;
    while (i < num_channels) {
      if (values[i]) {
        Serial.print(min(0xf, (int)(values[i])), HEX);
        end_vals[i] += (min(0xf, (int)(values[i])));
      }
      else {
        Serial.print(F("-"));
      }
      ++i;
    }
    Serial.println();
    delay(1);
  }
  uint8_t cfg = 0;
  File config = LittleFS.open("/jammerconf.txt", FILE_WRITE);
  if (!config) {
    Serial.println("Can't open conf file!");
    ESP.restart();
  }
  end_vals[num_channels] = '\0';
  for (uint8_t nz = 0; nz < 128; nz++) end_vals[nz] %= 128;
  if (config.print((char*)end_vals)) {
    Serial.println("Wrote config");
  }
}

byte i = 45;  ///CHANNEL NRF NEEDS TO START  37-50 CHANNEL

unsigned int flag = 0;

ezButton toggleSwitch(33);


void initSP() {
  sp = new SPIClass(VSPI);
  sp->begin();
  if (radio.begin(sp)) {
    delay(200);
    Serial.println("Sp Started !!!");
    radio.setAutoAck(false);
    radio.stopListening();
    radio.setRetries(0, 0);
    radio.setPayloadSize(255);   ////SET VALUE ON RF24.CPP
    radio.setAddressWidth(3);  ////SET VALUE ON RF24.CPP
    radio.setPALevel(RF24_PA_MAX, true);
    radio.setDataRate(RF24_2MBPS);
    radio.setCRCLength(RF24_CRC_DISABLED);
    radio.setAutoAck(false);
    radio.printPrettyDetails();
    radio.startConstCarrier(RF24_PA_MAX, i);  ////EDITED VALUES ON LIBRARY ....SET 5 BYTES PAYLOAD SIZE//REDUCE RF24_MAX TO RF24_HIGH OR LOW IF RF NOT STABLE
  } else {
    Serial.println("SP couldn't start !!!");
  }
}

bool scan_ch(uint8_t ch) {
  radio.setChannel(ch);
  uint32_t sum = 0;
  uint8_t buffer[128];
  // radio.flush_rx();
  // delay(150);
  radio.read(buffer, 128);
  for (uint8_t cnt = 0; cnt < 128; cnt++) sum += buffer[i];
  double level = sum / 128.0;
  Serial.println(level);
  return level;
}

void two() {

  ///CHANNEL WITH 2 SPACING HOPPING
  if (flag == 0) {
    i += 2;
  } else {
    i -= 2;
  }

  if ((i > 79) && (flag == 0)) {
    flag = 1;
  } else if ((i < 2) && (flag == 1)) {
    flag = 0;
  }

  radio.setChannel(i);
  Serial.println(scan_ch(i));
  //Serial.println(i);
}

void one() {
  ////SWEEP CHANNEL
  for (int i = 0; i < 79; i++) {
    radio.setChannel(i);
    Serial.println(scan_ch(i));
  }
}

/*YOU CAN DO RANDOM CHANNEL 


radio.setChannel(random(79));


*/


void setup(void) {
  uint8_t valuee;
  EEPROM.begin(512);
  EEPROM.get(2, valuee);
  if (valuee != 79) {
    full_scan();
    EEPROM.write(2, 79);
    EEPROM.commit();
    ESP.restart();
  }
  esp_bt_controller_deinit();
  esp_wifi_stop();
  esp_wifi_deinit();
  Serial.begin(115200);
  toggleSwitch.setDebounceTime(50);
  initSP();
}


void loop(void) {

  toggleSwitch.loop();  // MUST call the loop() function first






  /* if (toggleSwitch.isPressed())
    Serial.println("one");
  if (toggleSwitch.isReleased())
    Serial.println("two");*/

  int state = toggleSwitch.getState();

  delay(1);
  if (state != HIGH)
    two();

  else {
    one();
  }
}