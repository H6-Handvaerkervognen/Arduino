#include <BluetoothSerial.h>
#include <ArduinoJson.h>  //6.21.0
#include <WiFi.h>


#define PIN_VIBRATION 34
#define PIN_RED_LED 32
#define PIN_BLUE_LED 33
#define PIN_GREEN_LED 25

#define TONE_OUTPUT_PIN  21
const int TONE_PWM_CHANNEL = 0;

#define SSID "ZBC-E-CH-SKP019 0986"
#define PASSWORD "710%dK14"
#define DEVICE_PAIR_CODE "123456789"

enum State {
  IDLE,
  ALARM,
  PAIRING
};

BluetoothSerial SerialBT;
int currentState = IDLE;
int timer = 0;
const int seconds = 5;

int startHour = 0;
int startMinute = 0;
int endHour = 23;
int endMinute = 59;

int currentHour = 0;
int currentMinute = 0;

bool alarmOn = false;
bool devicePaired = false;

int melody[] = {  // note frequency
  262, 262, 262, 262, 262, 262, 262, 262
};
int noteDurations[] = {
  4, 8, 8, 4, 4, 8, 8, 4
};

struct tm timeinfo;


void setup() {
  Serial.begin(115200);
  pinMode(PIN_VIBRATION, INPUT);
  pinMode(PIN_RED_LED, OUTPUT);
  pinMode(PIN_BLUE_LED, OUTPUT);
  pinMode(PIN_GREEN_LED, OUTPUT);
  ledcAttachPin(TONE_OUTPUT_PIN, TONE_PWM_CHANNEL); 
  digitalWrite(PIN_RED_LED, HIGH);

  SerialBT.begin("VAN ALARM");
  connectToWiFi();
  configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");
}

void loop() {
  getLocalTimeInfo();
  connectToWiFi();
  switch (currentState) {
    case IDLE:
      digitalWrite(PIN_RED_LED, LOW);
      digitalWrite(PIN_BLUE_LED, HIGH);
      if (SerialBT.hasClient()) {
        currentState = PAIRING;
        digitalWrite(PIN_BLUE_LED, LOW);
        digitalWrite(PIN_GREEN_LED, HIGH);
      }

      break;
    case ALARM:
      detectionTimerRange();
      break;
    case PAIRING:
      if (!SerialBT.hasClient()) {
        currentState = IDLE;
        digitalWrite(PIN_GREEN_LED, LOW);
        digitalWrite(PIN_BLUE_LED, HIGH);
      } else {
        bluetoothReceive();
        detectVibration();
      }
      break;
  }
}

void detectVibration() {
  int value = analogRead(PIN_VIBRATION);
  //Serial.println(value);
  while (value > 2000 && timer < seconds) {
    delay(1000);
    value = analogRead(PIN_VIBRATION);
    //Serial.println(value);
    timer++;
  }
  if (timer == seconds) {
    currentState = ALARM;
  }
  timer = 0;
}



void bluetoothReceive() {
  char data[100];
  int i = 0;

  while (SerialBT.available() != 0) {
    char c = SerialBT.read();
    Serial.write(c);
    data[i] = c;
    i++;

    if(i >= 100){
      break;
    }

    if (SerialBT.available() == 0 && strstr(data, DEVICE_PAIR_CODE) != NULL && devicePaired == false) {
      devicePaired = true;
      bluetoothSendJsonEncode();
      i = 0;
    }
  }
}
void connectToWiFi() {
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

void bluetoothSendJsonEncode() {
  StaticJsonDocument<200> doc;
  doc["id"] = "123456789";
  doc["name"] = "VAN ALARM";
  serializeJson(doc, SerialBT);
  bluetoothSend("!");
}
void bluetoothSend(char *data) {
  if (data != NULL)
  {
    for (int i = 0; i < sizeof(data); i++) {
      SerialBT.write(data[i]);
  }
  }
}

void buzzer(int duration_ms) {
  int start_time = millis();
  while (millis() - start_time < duration_ms) {
    for (int i = 0; i < 8; i++) {
      int noteDuration = 1000 / noteDurations[i];
      ledcWriteTone(TONE_PWM_CHANNEL, melody[i]);
      delay(noteDuration);
      ledcWriteTone(TONE_PWM_CHANNEL, 0);
      delay(noteDuration);
    }
    if(alarmOn == false){
      start_time = millis() + duration_ms;
    }
  }
}
void getLocalTimeInfo(){
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  else{
    currentHour = timeinfo.tm_hour;
    currentMinute = timeinfo.tm_min;
  }
}

void detectionTimerRange(){
  if(currentHour >= startHour && currentHour <= endHour){ 
    if(currentMinute >= startMinute && currentMinute <= endMinute){
      Serial.println("############## ALARM!!! ##############");
      //buzzer();
      currentState = IDLE;
      digitalWrite(PIN_GREEN_LED, LOW);
      digitalWrite(PIN_BLUE_LED, LOW);
      digitalWrite(PIN_RED_LED, HIGH);
      delay(1000);
    }
  }
}