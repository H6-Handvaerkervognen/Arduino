#include <BluetoothSerial.h>

#define PIN_VIBRATION 34

enum State{
  IDLE,
  ALARM,
  PAIRING
};
BluetoothSerial SerialBT;
int currentState = IDLE;
int timer = 0;
const int seconds = 5;

void setup() {
  Serial.begin(9600);
  SerialBT.begin();
  pinMode(PIN_VIBRATION, INPUT);
}

void loop() {
  switch(currentState){
    case IDLE:
      if (SerialBT.hasClient()) 
      {
        bluetoothSend();
        detectVibration();
      }
      break;
    case ALARM:
      Serial.println("############## ALARM!!! ##############");
      currentState = IDLE;
      break;
  }
}

void detectVibration()
{
  int value = analogRead(PIN_VIBRATION);
  Serial.println(value);
  while(value > 2000 && timer < seconds)
  {
    delay(1000);
    value = analogRead(PIN_VIBRATION);
    Serial.println(value);
    timer++;
  }
  if(timer == seconds)
  {
    currentState = ALARM;
  }
  timer = 0;
}

void bluetoothSend()
{
  char data[] = "Hello World\n";
  for (int i = 0; i < sizeof(data); i++) {
    SerialBT.write(data[i]);
  }
}
