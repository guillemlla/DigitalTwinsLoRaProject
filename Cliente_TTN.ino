#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include "Adafruit_Si7021.h"

const uint8_t PINPRESENCE = 1;
const uint8_t PINLIGHT = 0;

uint8_t presence = 0;

Adafruit_Si7021 tempHumSensor = Adafruit_Si7021();


//LoRaWAN Session Key
 u1_t NWKSKEY[16] = { 0xDE, 0x94, 0xE5, 0xED, 0x2E, 0xAF, 0xA9, 0x91, 0x3A, 0x71, 0x73, 0x2B, 0xA9, 0xDF, 0x52, 0x73 };

// LoRaWAN AppSKey
u1_t  APPSKEY[16] = { 0x42, 0x4E, 0x6D, 0xA9, 0x87, 0x57, 0x79, 0x6A, 0xBB, 0xA1, 0x15, 0x70, 0xAB, 0xDA, 0x65, 0x9C };

// LoRaWAN end-device address
 u4_t DEVADDR = 0x26011241;

void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static uint8_t mydata[7];;
static osjob_t sendjob;

//Every how many seconds you would like to send a messaged
//This is not guarenteed and depends on the GW the maximum
//number of messages
const unsigned TX_INTERVAL = 150;

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 9,
    .dio = {2, 6, 7},
};

void onEvent (ev_t ev) {
    Serial.println(os_getTime());
    Serial.println(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            break;
        case EV_RFU1:
            Serial.println(F("EV_RFU1"));
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if(LMIC.dataLen) {
                Serial.println(F("Data Received: "));
                Serial.write(LMIC.frame+LMIC.dataBeg, LMIC.dataLen);
                Serial.println();
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
         default:
            Serial.println(F("Unknown event"));
            break;
    }
}

void do_send(osjob_t* j){
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        setData();
        LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
        Serial.println(F("Packet queued"));
    }
}

void setup() {
    Serial.begin(9600);
    Serial.println(F("Starting"));

    if (!tempHumSensor.begin()) {
      Serial.println("Did not find Si7021 sensor!");
      while (true);
    }

    #ifdef VCC_ENABLE
    // For Pinoccio Scout boards
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
    delay(1000);
    #endif

    os_init();
    LMIC_reset();

    #ifdef PROGMEM

    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
    #else

    LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
    #endif

    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
    LMIC_setLinkCheckMode(0);
    LMIC_setDrTxpow(DR_SF7,14);

    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}

void setData(){

  Serial.println("_______________________");
  Serial.println("Begin Setting Data");

  uint8_t humVal = tempHumSensor.readHumidity();
  float tempVal = tempHumSensor.readTemperature();
  uint8_t tempIntVal = tempVal;
  uint8_t tempDecVal = (tempVal-tempIntVal)*100;

  checkPIR();
  uint8_t presenceVal = presence;
  presence = 0;

  int lightVal = analogRead(PINLIGHT);
  uint8_t lightHighVal = lightVal>>8;
  uint8_t lightLowVal = lightVal & 0xFF;

  mydata[0] = humVal;
  mydata[1] = tempIntVal;
  mydata[2] = tempDecVal;
  mydata[3] = presenceVal;
  mydata[4] = lightHighVal;
  mydata[5] = lightLowVal;

  Serial.println("Data prepared");
  Serial.print("Hum=");
  Serial.print(humVal);
  Serial.print(" Temp=");
  Serial.print(tempIntVal);
  Serial.print(",");
  Serial.print(tempDecVal);
  Serial.print(" Presence=");
  Serial.print(presenceVal);
  Serial.print(" Light=");
  Serial.println(lightVal);

}

void checkPIR(){

  if(analogRead(PINPRESENCE)>500){
    presence= 1;
  }
}
