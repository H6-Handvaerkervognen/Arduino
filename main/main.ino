#include <BluetoothSerial.h>
#include <ArduinoJson.h>  //6.21.0
#include <WiFi.h>


#define PIN_VIBRATION 34
#define PIN_RED_LED 32
#define PIN_BLUE_LED 33
#define PIN_GREEN_LED 25

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

void setup() {
  Serial.begin(115200);
  pinMode(PIN_VIBRATION, INPUT);
  pinMode(PIN_RED_LED, OUTPUT);    // RED
  pinMode(PIN_BLUE_LED, OUTPUT);   // BLUE
  pinMode(PIN_GREEN_LED, OUTPUT);  // GREEN
  digitalWrite(PIN_RED_LED, HIGH);

  SerialBT.begin("VAN ALARM");
}

void loop() {
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

void bluetoothSend() {
  delay(1000);
  char data[] = "Hello World!";
  for (int i = 0; i < sizeof(data); i++) {
    SerialBT.write(data[i]);
  }
}

void bluetoothReceive() {
  while (SerialBT.available() != 0) {
    char c = SerialBT.read();
    Serial.write(c);
     if(SerialBT.available() == 0)
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
}
