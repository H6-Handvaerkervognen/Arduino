
#define PIN_VIBRATION 34

enum State{
  IDLE,
  ALARM,
  PAIRING
};
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
      detectVibration();
      break;
    case ALARM:
      Serial.println("############## ALARM!!! ##############");
      currentState = IDLE;
      break;
  }
}

void detectVibration()
{
  int value = analogRead(pinVibration);
  Serial.println(value);
  while(value > 2000 && timer < seconds)
  {
    delay(1000);
    value = analogRead(pinVibration);
    Serial.println(value);
    timer++;
  }
  if(timer == seconds)
  {
    currentState = ALARM;
  }
  timer = 0;
}
