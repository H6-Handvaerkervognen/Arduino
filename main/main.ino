#define pinVibration 34
int timer = 0;
int seconds = 5;

void setup() {
  Serial.begin(9600);
  pinMode(pinVibration, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:

void detectVibration()
{
  int value = analogRead(pinVibration);
  Serial.println(value);
  while(value > 2000 && timer != seconds)
  {
    delay(1000);
    value = analogRead(pinVibration);
    Serial.println(value);
    timer++;
    if(timer == seconds)
    {
    }
  }
  timer = 0;
}
