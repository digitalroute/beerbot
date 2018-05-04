#include <ESP8266WiFi.h>
#define PubNub_BASE_CLIENT WiFiClient
#define PUBNUB_DEFINE_STRSPN_AND_STRNCASECMP
#include <PubNub.h>


#define OPEN 0
#define CLOSE 2

void setup() {
  Serial.begin(115200);
  Serial.println("Starting");
  pinMode(OPEN, OUTPUT);
  pinMode(CLOSE, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {                         
  openValve();
  delay(8000);
  closeValve();
  delay(8000);
}

void openValve() {
  Serial.println("Opening valve");
  digitalWrite(CLOSE, LOW);                              
  delay(200);                    
  digitalWrite(OPEN, HIGH);
  delay(4000);
  digitalWrite(OPEN, LOW);
  Serial.println("Opened");
}

void closeValve() {
  Serial.println("Closing valve");
  digitalWrite(OPEN, LOW);                              
  delay(200);                    
  digitalWrite(CLOSE, HIGH);
  delay(4000);
  digitalWrite(CLOSE, LOW);
  Serial.println("Closed");
}

