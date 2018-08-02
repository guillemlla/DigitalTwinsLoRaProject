#include <SPI.h>
#include <RH_RF95.h>
#include "Adafruit_Si7021.h"

RH_RF95 rf95;

float frequency = 868.0;
uint8_t id = 0;
uint8_t presence = 0;

const uint8_t REQNEWID = 0;
const uint8_t SETNEWID = 1;
const uint8_t SENDDATA = 2;
const uint8_t ACK = 3;
const uint8_t NACK = 4;

const uint8_t PINPRESENCE = 1;
const uint8_t PINLIGHT = 0;

Adafruit_Si7021 tempHumSensor = Adafruit_Si7021();

//Configures all the parameters needed to beging to send LoRa messages
void setup(){
  Serial.begin(9600);
  while (!Serial) ; // Wait for serial port to be available

  if (!tempHumSensor.begin()) {
    Serial.println("Did not find Si7021 sensor!");
    while (true);
  }

  Serial.println("Start LoRa Client");

  if (!rf95.init()) Serial.println("init failed");

  rf95.setFrequency(frequency);
  rf95.setTxPower(13);
  // Setup Spreading Factor (6 ~ 12)
  rf95.setSpreadingFactor(7);
  // Setup BandWidth, option: 7800,10400,15600,20800,31250,41700,62500,125000,250000,500000
  //Lower BandWidth for longer distance.
  rf95.setSignalBandwidth(125000);
  // Setup Coding Rate:5(4/5),6(4/6),7(4/7),8(4/8)
  rf95.setCodingRate4(5);
  initilize();
}

//Manages everything to initilize the node with a new id
void initilize(){

  uint8_t data[1];
  data[0] = REQNEWID;

  //Send the request of connection
  rf95.send(data, sizeof(data));
  rf95.waitPacketSent();
  // Now wait for a reply

  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  Serial.println("Send Request Inizialization");

  if (rf95.waitAvailableTimeout(10000)){
    // Should be a reply message for us now
    if (rf95.recv(buf, &len)){
      Serial.print("Got reply: ");
      Serial.println((char*)buf);
      //If the reply is correct and is giving us an id
      if(buf[0]==SETNEWID){
        id = buf[1];
        int delayTime = (buf[3]<<8) + buf[4];
        Serial.print("ID= ");
        Serial.print(id);
        Serial.print(" Slot=");
        Serial.print(buf[2]);
        Serial.print(" Delay=");
        Serial.print(delayTime);
        Serial.println("s");
        //sleep the amount of seconds that the gateway said
        delaySeconds(delayTime);
        Serial.println("End up");
      }else{
        Serial.println("Error Data");
        delay(5000);
        initilize();
      }
    }
    else{
      Serial.println("recv failed");
      initilize();
    }
  }
  else{
    Serial.println("No reply, is LoRa server running?");
    initilize();
  }
  delay(5000);
}

void loop(){
  int delayValue = sendData();
  delaySeconds(delayValue);
}

//Gets the data from the sensors and calls the function responsable of getting
//getting the response
int sendData(){
  Serial.println("_______________________");
  Serial.println("Begin Sending Data");

  uint8_t humVal = tempHumSensor.readHumidity();
  float tempVal = tempHumSensor.readTemperature();
  //To send the temperature that is a float. I divided it into its integer part
  //and two of its decimal numbers. And I send each part separatelly
  uint8_t tempIntVal = tempVal;
  uint8_t tempDecVal = (tempVal-tempIntVal)*100;
  checkPIR();
  uint8_t presenceVal = presence;
  presence = 0;

  int lightVal = analogRead(PINLIGHT);
  //The lightValue can be more than 256, so we have to devided into two bytes
  uint8_t lightHighVal = lightVal>>8;
  uint8_t lightLowVal = lightVal & 0xFF;

  uint8_t data[8];
  data[0] = SENDDATA;
  data[1] = id;
  data[2] = humVal;
  data[3] = tempIntVal;
  data[4] = tempDecVal;
  data[5] = presenceVal;
  data[6] = lightHighVal;
  data[7] = lightLowVal;

  Serial.println("Data to send");
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

  rf95.send(data, sizeof(data));
  rf95.waitPacketSent();
  Serial.println("Send");

  // Now wait for a reply
  return recieveDataMessage();

}

//Analyzes the recieved messages and returns the delayed obtained if
//the messaged from the GW is recieved correctly, if not it will be 300s
int recieveDataMessage(){
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  if (rf95.waitAvailableTimeout(3000)){
    // Should be a reply message for us now
    if (rf95.recv(buf, &len)){
      Serial.print("got reply: ");
      Serial.println((char*)buf);
      int delayTime = -1;
      //Check if is the correct message with our id
      if(buf[0]==ACK && buf[1]==id){
        delayTime = (buf[2]<<8) + buf[3];
        Serial.println("Delay Time:"+String(delayTime));
        return delayTime;
      }else{
        recieveDataMessage();
      }
    }
    else{
      Serial.println("recv failed");
      //if no message recieved wait 300s by default
      return 300;
    }
  }else{
    Serial.println("No reply, is LoRa server running?");
    //if no message recieved wait 300s by default
    return 300;
  }
}
//This function is created because with big delays the delay function is not
//working properly, and also to be able to check the PIR sensor every 10s
void delaySeconds(int s){
  int num10 = s/10;
  int resto = s%10;
  checkPIR();
  for(int i =0;i<num10;i++){
    delay(10000);
    checkPIR();
  }
  delay(resto*1000);
  checkPIR();
}

void checkPIR(){
  if(analogRead(PINPRESENCE)>500){
    presence= 1;
  }
}
