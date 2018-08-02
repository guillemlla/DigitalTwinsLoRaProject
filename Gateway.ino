
#include <SPI.h>
#include <RH_RF95.h>
#include <FileIO.h>
#include <Console.h>
#include <Process.h>
#include <Bridge.h>

RH_RF95 rf95;

int led = A2;
float frequency = 868.0;

const uint8_t NUMIDS = 10;
const uint8_t TIMESLOT = 30; //(s)
const int CICLETIME = NUMIDS*TIMESLOT;
int ids[NUMIDS] = {-1};
long timeslots[NUMIDS] = {false};
//If every slot is 30s (0,30,60,90,120,...270)

//Possible messages recieved and send
const uint8_t REQNEWID = 0;
const uint8_t SETNEWID = 1;
const uint8_t SENDDATA = 2;
const uint8_t ACK = 3;
const uint8_t NACK = 4;


void setup(){
  pinMode(led, OUTPUT);
  Bridge.begin(115200);
  if (!rf95.init()) Console.println("init failed");

  rf95.setFrequency(frequency);
  rf95.setTxPower(13);

  Console.print("Listening on frequency: ");
  Console.println(frequency);
}

void loop(){
  if (rf95.available()){
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (rf95.recv(buf, &len)){
      digitalWrite(led, HIGH);
      RH_RF95::printBuffer("request: ", buf, len);
      Console.println("____________________________");
      Console.print("got request. State:");
      Console.println(buf[0]);

      if(buf[0] == REQNEWID){
        //Request of a new device
        initialize();
      }else if(buf[0] == SENDDATA && ids[buf[1]]!=-1){
        //Is a registered device sending data
        saveData(buf);
      }
    }
    else{
      Console.println("recv failed");
    }
  }

void initialize(){

      uint8_t data[5];
      //The first byte corresponds to the type of message
      data[0] = SETNEWID;

      uint8_t i = 0;
      //Get the first timeslot that is false
      while(timeslots[i]){i++;}
      //Set that to occupied
      timeslots[i] = true;
      //Value of the beggining of the timeslot
      int timeSlotVal = i*TIMESLOT;

      i = 0;
      //Get the first free id
      while(ids[i]!=-1){i++;}
      //Set the id to the value of the timeSlot
      ids[i] = timeSlotVal;
      //The second byte is the id of the new device
      data[1] = i;

      //Calculate the delay that should be send to the node regarding its slot
      int currentCicleProgress = 0;
      long sec = millis();
      sec = sec/1000;
      //Calculate the progress of the current cicle
      if(sec>CICLETIME){
        currentCicleProgress = sec%CICLETIME;
      }else{
        currentCicleProgress = sec;
      }
      int delayValue = 0;
      //Calculate the delay leaving a small window of TIMESLOT/2
      if(currentCicleProgress<timeSlotVal){
        delayValue = timeSlotVal - currentCicleProgress + (TIMESLOT/2);
      }else if(currentCicleProgress > timeSlotVal){
        delayValue = timeSlotVal + CICLETIME - currentCicleProgress + (TIMESLOT/2);
      }

      Console.print("TimeSlotVal: ");
      Console.println(timeSlotVal);
      Console.print("currentCicleProgress: ");
      Console.println(currentCicleProgress);
      Console.print("delayValue: ");
      Console.println(delayValue);

      data[2] = timeSlotVal/TIMESLOT;
      //The delay can be more than 256, so we have to devided into two bytes
      data[3] = delayValue>>8;
      data[4] = delayValue & 0xFF;

      rf95.send(data, sizeof(data));
      rf95.waitPacketSent();
      Console.println("Sent a reply");
      digitalWrite(led, LOW);

      FileSystem.begin();
      File file_name = FileSystem.open("/etc/dataiot/data3",FILE_WRITE);
      file_name.seek(file_name.size());
      String s = "New connection, id= "+ String(data[1]);
      file_name.println(s);
      file_name.close();
}

void saveData(uint8_t data[]){

  int lightHighVal2 = data[6]<<8;
  int lightVal2 = lightHighVal2 + data[7];

  float temp = data[3] + data[4]/100;

  char idValS[10];char humValS[10];char tempIntValS[10]; char tempDecValS[10]; char presenceValS[10];char lightValS[10];
  itoa(data[1],idValS,10);
  itoa(data[2],humValS,10);
  itoa(data[3],tempIntValS,10);
  itoa(data[4],tempDecValS,10);
  itoa(data[5],presenceValS,10);
  itoa(lightVal2,lightValS,10);

  String dataS = String(millis())+" "+String(idValS)+":"+String(humValS)+":"+String(tempIntValS)+","+String(tempDecValS)+":"+String(presenceValS)+":"+String(lightValS)+"\0";
  Console.println("Data recieved: "+dataS);
  FileSystem.begin();
  File file_name = FileSystem.open("/etc/dataiot/data3",FILE_WRITE);
  file_name.seek(file_name.size());
  file_name.println(dataS);
  file_name.close();

  int timeSlotVal = ids[data[1]];
  long sec = millis();
  sec = sec/1000;
  int currentCicleProgress = 0;
  if(sec>CICLETIME){
    Console.println("Dentro de millis > cicletime*1000");
    currentCicleProgress = sec%CICLETIME;
  }else{
    Console.println("Dentro de millis < cicletime*1000");
    currentCicleProgress = sec;
  }
  int delayValue = 0;
  Console.print("milis:" );
  Console.println(millis());
  Console.print("TimeSlotVal: ");
  Console.println(timeSlotVal);
  Console.print("currentCicleProgress: ");
  Console.println(currentCicleProgress);


  if(currentCicleProgress<timeSlotVal){
    delayValue = timeSlotVal - currentCicleProgress + (TIMESLOT/2);
  }else if(currentCicleProgress > timeSlotVal){
    delayValue = timeSlotVal + CICLETIME - currentCicleProgress + (TIMESLOT/2);
  }
  Console.print("delayValue: ");
  Console.println(delayValue);


  // Send a reply
  uint8_t responseData[4];
  responseData[0] = ACK;
  responseData[1] = data[1];
  responseData[2] = delayValue>>8;
  responseData[3] = delayValue & 0xFF;
  Console.print("Delay1: ");
  Console.println(String(responseData[2]));
  Console.print("Delay2: ");
  Console.println(String(responseData[3]));
  rf95.send(responseData, sizeof(responseData));
  rf95.waitPacketSent();
  Console.println("Sent a reply");
  digitalWrite(led, LOW);

}
