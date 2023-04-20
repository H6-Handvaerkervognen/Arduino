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
  pinMode(PIN_RED_LED, OUTPUT);    // RED
  pinMode(PIN_BLUE_LED, OUTPUT);   // BLUE
  pinMode(PIN_GREEN_LED, OUTPUT);  // GREEN
  ledcAttachPin(TONE_OUTPUT_PIN, TONE_PWM_CHANNEL);
  digitalWrite(PIN_RED_LED, HIGH);

  SerialBT.begin("VAN ALARM");
  buzzer();
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
      Serial.println("############## ALARM!!! ##############");
      currentState = IDLE;
      digitalWrite(PIN_GREEN_LED, LOW);
      digitalWrite(PIN_BLUE_LED, LOW);
      digitalWrite(PIN_RED_LED, HIGH);
      delay(1000);
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
  while (SerialBT.available() != 0) {
    char c = SerialBT.read();
    Serial.write(c);
    if (SerialBT.available() == 0)
    {
      bluetoothSendJsonEncode();
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
  bluetoothSend();
}

void bluetoothSend() {
  char data[] = "!";
  for (int i = 0; i < sizeof(data); i++) {
    SerialBT.write(data[i]);
  }
}

void buzzer()
{
  for (int thisNote = 0; thisNote < 8; thisNote++) {
    int noteDuration = 1000 / noteDurations[thisNote];
    ledcWriteTone(TONE_PWM_CHANNEL, melody[thisNote]);
    delay(noteDuration);
    ledcWriteTone(TONE_PWM_CHANNEL, 0);
    delay(noteDuration);
  }
void getLocalTimeInfo(){
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  currentHour = timeinfo.tm_hour;
  currentMinute = timeinfo.tm_min;
}
