#include <BluetoothSerial.h>
#include <ArduinoJson.h>  //6.21.0
#include <WiFi.h>
#include <HTTPClient.h>
#include <RCSwitch.h>
#include <WiFiClientSecure.h>

#define DEVICE_ID 1
#define DEVICE_NAME "VAN ALARM"

#define PIN_VIBRATION 34
#define PIN_RED_LED 32
#define PIN_BLUE_LED 33
#define PIN_GREEN_LED 25
#define PIN_YELLOW_LED 26

#define RC_SWITCH_PIN 16

#define BUTTON_BLUE 22 
#define BUTTON_BLACK 19 

#define TONE_OUTPUT_PIN  21
const int TONE_PWM_CHANNEL = 0;

#define SSID "Linksys00082"
#define PASSWORD "h@ndVaerk58"
#define DEVICE_PAIR_CODE "12345"

enum State {
  IDLE,
  ALARM,
  PAIRING
};

BluetoothSerial SerialBT;
WiFiClientSecure client;
HTTPClient https;
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
bool threadCreated = false;

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
  pinMode(PIN_YELLOW_LED, OUTPUT);
  pinMode(BUTTON_BLUE, INPUT_PULLUP);
  pinMode(BUTTON_BLACK, INPUT_PULLUP);
  ledcAttachPin(TONE_OUTPUT_PIN, TONE_PWM_CHANNEL);
  digitalWrite(PIN_RED_LED, HIGH);

  SerialBT.begin("VAN ALARM");
  connectToWiFi();
  configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");

  Serial.println("GET");
  httpsRequest("https://192.168.1.11/Alarm/GetAlarmInfo?alarmId=1", "GET", "application/json");
}

void loop() {
  connectToWiFi();
  getLocalTimeInfo();
  switch (currentState) {
    case IDLE:
      digitalWrite(PIN_RED_LED, LOW);
      digitalWrite(PIN_BLUE_LED, HIGH);
      digitalWrite(PIN_GREEN_LED, LOW);
      digitalWrite(PIN_YELLOW_LED, LOW);
      while (devicePaired == false)
      {
        if (SerialBT.hasClient())
        {
          bluetoothReceive();
        }
      }
      break;
    case ALARM:
      detectionTimerRange();
      break;
    case PAIRING:
      digitalWrite(PIN_YELLOW_LED, LOW);
      digitalWrite(PIN_RED_LED, LOW);
      digitalWrite(PIN_BLUE_LED, LOW);
      digitalWrite(PIN_GREEN_LED, HIGH);
      if (devicePaired == true)
      {
        readButton(BUTTON_BLUE);
        readButton(BUTTON_BLACK);
        if(alarmOn == false)
        {
          detectVibration();
        }  
      }
      break;
  }
}

/* Read button state
    Read the button state
    If the blue button is pressed, change state to IDLE
    If the black button is pressed, restart the device to factory settings
*/
void readButton(int button)
{
  int buttonState = digitalRead(button);
  if (buttonState == 0 && alarmOn == false)
  {
    if(button == BUTTON_BLUE)
    {
      Serial.println("Blue button pressed");
      currentState = IDLE;
      devicePaired = false;
    }
    else if(button == BUTTON_BLACK)
    {
      Serial.println("Black button pressed");
      httpsRequest("https://192.168.1.11/Alarm/DeletePairing?alarmID=1", "DELETE", "application/json");
      ESP.restart();
    }
  }
}
/* Detect vibration
    Read the vibration sensor value
    If the value is greater than 2000 for 5 seconds, change state to ALARM
*/
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

/* Receive data via Bluetooth
    Read the data from the client one character at a time
    If the data contains the device pair code
    Send the device information to the client
*/
void bluetoothReceive() {
  char data[100];
  int i = 0;

  while (SerialBT.available() != 0) {
    char c = SerialBT.read();
    Serial.write(c);
    data[i] = c;
    i++;

    if (i >= 100) {
      break;
    }

    if (SerialBT.available() == 0 && strstr(data, DEVICE_PAIR_CODE) != NULL && devicePaired == false) {
      devicePaired = true;
      currentState = PAIRING;
      bluetoothSendJsonEncode();
      i = 0;
    }
  }
}
/* Connect to WiFi
    If not already connected, connect to WiFi
    Retry every 500ms until connected
*/
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
/* Send JSON via Bluetooth
    Create JSON object and send it to the client
    Send a "!" to indicate the end of the JSON data
    This is used by the client to know when to stop reading
*/
void bluetoothSendJsonEncode() {
  StaticJsonDocument<200> doc;
  doc["id"] = DEVICE_ID;
  doc["name"] = DEVICE_NAME;
  serializeJson(doc, SerialBT);
  bluetoothSend("!");
}
/* Send text via Bluetooth
    Send a char array one character at a time
*/
void bluetoothSend(char *data) {
  if (data != NULL)
  {
    for (int i = 0; i < sizeof(data); i++) {
      SerialBT.write(data[i]);
    }
  }
}
/* Buzzer alarm
    Play the buzzer for the duration of the alarm
    If the alarm is turned off then stop playing the buzzer
*/
void buzzer(int duration_ms) {
  unsigned long start_time = millis();
  while (millis() - start_time < duration_ms) {
    for (int i = 0; i < 8; i++) {
      int noteDuration = 1000 / noteDurations[i];
      //ledcWriteTone(TONE_PWM_CHANNEL, melody[i]);
      delay(noteDuration);
      //ledcWriteTone(TONE_PWM_CHANNEL, 0);
      delay(noteDuration);
    }
    if (alarmOn == false) {
      start_time = millis() + duration_ms;
    }
  }
}
/* Get local time
    Get the current time from the NTP server
    If the time is obtained, set the current hour and minute
*/
void getLocalTimeInfo() {
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  else {
    currentHour = timeinfo.tm_hour;
    currentMinute = timeinfo.tm_min;
  }
}
/* Detection timer range
    Check if the current time is within the timer range
    If it is, start the alarm sound and send a http request
    Create 2 threads
    thred 1 for sending http request
    thread 2 for reading the RC switch
*/
void detectionTimerRange() {
  if ((currentHour > startHour || (currentHour == startHour && currentMinute >= startMinute))
    && ((currentHour < endHour || (currentHour == endHour && currentMinute <= endMinute)) || (endHour < startHour && (currentHour < endHour || currentHour == endHour && currentMinute <= endMinute)))
    || (currentHour == endHour && currentMinute <= endMinute && endHour >= startHour)
    || (currentHour == startHour && currentMinute >= startMinute && endHour >= startHour)) {
      Serial.println("############## ALARM!!! ##############");
      alarmOn = true;
      httpsRequest("https://192.168.1.11/Alarm/ActivateAlarm", "POST", "application/json","{\"alarmID\": \"1\"}");
      digitalWrite(PIN_YELLOW_LED, HIGH);
      digitalWrite(PIN_RED_LED, LOW);
      digitalWrite(PIN_BLUE_LED, LOW);
      digitalWrite(PIN_GREEN_LED, LOW);


      if (!threadCreated) {
        threadCreated = false;
        Serial.println("Creating thread");
        int err = xTaskCreatePinnedToCore(sendHttpRequest, "sendHttpRequest", 3072, NULL, 1, NULL, 1);
        Serial.println(err);
        int err2 = xTaskCreatePinnedToCore(rcRead, "rcRead", 2048, NULL, 1, NULL, 1);
        Serial.println(err2);
        threadCreated = true;
        buzzer(18000);
      }
      currentState = PAIRING;
  }
}

/* Send HTTP request thread function
    Send a GET request to the api every second to check if the alarm is turned off
    If the alarm is turned off, stop the thread
*/
void sendHttpRequest(void * parameter) {
  Serial.println("Thread 2");
  int duration_s = 180;
  for (size_t i = 0; i < duration_s; i++)
  {
    if (alarmOn == false) {
      threadCreated = false;
      vTaskDelete(NULL);
    }
    Serial.println("Time left: " + String(duration_s - i));
    httpsRequest("https://192.168.1.11/Alarm/GetStatus?alarmID=1", "GET", "application/json");
    vTaskDelay(1000);
  }
  Serial.println("Task done!");
  threadCreated = false;
  vTaskDelete(NULL);
}

/* RC Read thread function
    Read the RC switch value
    If the value is from the paird remote
    Turn off the alarm
*/
void rcRead(void * parameter)
{
  Serial.println("Thread 3");
  RCSwitch mySwitch = RCSwitch();
  mySwitch.enableReceive(RC_SWITCH_PIN);
  int duration_s = 180;
  for (int i = 0; i < duration_s; i++)
  {
    if (alarmOn == false)
    {
      vTaskDelete(NULL);
    }

    if (mySwitch.available())
    {
      int value = mySwitch.getReceivedValue();
      Serial.println(value);
      if (value == 6941604 || value == 6941601 || value == 6941608 || value == 6939554)
      {
        alarmOn = false;
        Serial.println("Alarm off");
        vTaskDelete(NULL);
      }
    }
    vTaskDelay(1000);
  }
}

/* Send HTTPS request
    Send a GET request to the specified URL
    If the response contains "false", turn off the alarm
    If the response contains "timeStart" and "timeEnd", set the timer range

*/
void httpsRequest(char* url, char* requestType, char* content) {
  Serial.println("Sending request...");
  char* response;
  if (WiFi.status() == WL_CONNECTED)
  {
    char* certificate;
    client.setCACert(certificate);
    client.setInsecure();
    https.begin(client, url);
    https.addHeader("Content-Type", content);
    if (requestType == "GET")
    {
      int httpResponseCode = https.GET();
      if(httpResponseCode == 200)
      {
        Serial.println("Request sent!");
        String response = https.getString();
        Serial.println(response);
        if(strstr(response.c_str(), "false") != NULL)
        {
          Serial.println("Alarm is off!");
          alarmOn = false;
        }
        else if(strstr(response.c_str(), "startTime") != NULL && strstr(response.c_str(), "endTime") != NULL)
        {

          StaticJsonDocument<200> doc;
          DeserializationError error = deserializeJson(doc, response);
          if (error) {
            Serial.println("deserializeJson() failed: ");
            Serial.println(error.f_str());
            return;
          }
          const char* startTime = doc["startTime"];
          const char* endTime = doc["endTime"];

          char* startHourChar = strtok((char*)startTime, ":");
          char* startMinuteChar = strtok(NULL, ":");
          char* endHourChar = strtok((char*)endTime, ":");
          char* endMinuteChar = strtok(NULL , ":");

          startHour = atoi(startHourChar);
          startMinute = atoi(startMinuteChar);
          endHour = atoi(endHourChar);
          endMinute = atoi(endMinuteChar);
        }
      }
      else
      {
        Serial.println("Error on sending GET: " + httpResponseCode);
      }
    }
    else if(requestType == "DELETE")
    {
      int httpResponseCode = https.sendRequest(requestType);
      if(httpResponseCode == 200)
      {
        Serial.println("Request sent!");
        String response = https.getString();
        Serial.println(response);
      }
      else
      {
        Serial.println("Error on sending DELETE:");
        Serial.print(httpResponseCode);
      }
    }
    else
    {
      Serial.println("Error on sending request: ");
      Serial.print(requestType);
    }
    https.end();
    Serial.println("Request ended!");
  }
}

void httpsRequest(char* url, char* requestType, char* content, char* data) {
  Serial.println("Sending request...");
  if (WiFi.status() == WL_CONNECTED)
  {
    char* certificate;
    client.setCACert(certificate);
    client.setInsecure();
    https.begin(client, url);
    https.addHeader("Content-Type", content);
    https.addHeader("Host","192.168.1.146");
    https.addHeader("Content-Length","24");
    if (requestType == "POST")
    {
      int httpResponseCode = https.POST(data);
      Serial.println(httpResponseCode);
      if(httpResponseCode == 200)
      {
        Serial.println("Request sent!");
        String response = https.getString();
        Serial.println(response);
      }
      else
      {
        Serial.println("Error on sending POST:");
        Serial.print(httpResponseCode);
      }
    }
    else
    {
      Serial.println("Error on sending request: ");
      Serial.print(requestType);
    }
    https.end();
    Serial.println("Request ended!");
  }
}
