
/*
 *  Biruino template
 *
 */
#include <SPI.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#define PubNub_BASE_CLIENT WiFiClient
#include <PubNub.h>
#include "secrets.h"

ESP8266WiFiMulti WiFiMulti;

const static char channel[] = "flow"; // channel to use

char message[256];

byte sensorPin = 12;  // input pin
//byte interruptPin = 2;
volatile int pulseCount = 0;
unsigned long totalCount = 0;
int sendZeroUsage = 0;

volatile int pingNow;
unsigned long oldTime;
unsigned long feedbackTimer;
int ledStatus;
int feedback;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.print("setup()");

  pinMode(LED_BUILTIN, OUTPUT);

  // We start by connecting to a WiFi network
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(SECRET_SSID, SECRET_SSID_PASSWORD);

  Serial.println();
  Serial.println();
  Serial.print("Wait for WiFi... ");

  while(WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  delay(500);

  PubNub.begin(SECRET_PUBKEY, SECRET_SUBKEY);
  Serial.println("PubNub set up");

  oldTime = 0;
  feedbackTimer = 0;
  pingNow = 10000; // make sure to send a ping first :)
  ledStatus = 0;
  feedback = 0;
  ping();

  pinMode(sensorPin, INPUT_PULLUP);
  digitalWrite(sensorPin, HIGH);
  //attachInterrupt(interruptPin, pulseCounter, FALLING);
  attachInterrupt(digitalPinToInterrupt(sensorPin), pulseCounter, FALLING);
}

void loop() {
  if((millis() - oldTime) > 1000) {
    oldTime = millis();

    ping();
    if (feedback == 0) {
      toggleLed();
    }
  }

  if ((millis() - feedbackTimer) > 200 && feedback > 0) {
    toggleLed();
    feedback--;
  }



  if (pulseCount > 0) {
    createMessage(message, pulseCount, totalCount);
    publish(message);
    sendZeroUsage = 1;
  } else {
    if (sendZeroUsage > 0) {
      createMessage(message, 0, totalCount);
      publish(message);
      sendZeroUsage = 0;
    }
  }

  
}

void toggleLed() {
  if (ledStatus > 0) {
    digitalWrite(LED_BUILTIN, LOW);
    ledStatus = 0;
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
    ledStatus = 1;    
  }
}

void ping() {
  if (pingNow >= 30) {
    Serial.println("Sending ping");
    createPing(message);
    publish(message);
    pingNow = 0;
  }
  
  pingNow++;
}

void createMessage(char* s, int usage, int total) {
  sprintf(s, "{\"usage\": \"%d\",\"source\": \"flowmeter\",\"type\": \"counter\",\"uptime\": \"%d\"}", usage, millis() / 1000);
}

void createPing(char* s) {
  sprintf(s, "{\"source\": \"flowmeter\",\"type\": \"ping\",\"uptime\": \"%d\"}", millis() / 1000);
}

void publish(char* msg) {
  WiFiClient *client;
  
  client = PubNub.publish(channel, msg);

  if (!client) {
    //Serial.println("publishing error");
    delay(1000);
    return;
  }
  if (PubNub.get_last_http_status_code_class() != PubNub::http_scc_success) {
    Serial.print("Got HTTP status code error from PubNub, class: ");
    Serial.print(PubNub.get_last_http_status_code_class(), DEC);
  }
  while (client->available()) {
    Serial.write(client->read());
  }
  client->stop();
  Serial.println("---");
}

void pulseCounter() {
  pulseCount++;
  totalCount++;
  //Serial.print("pulse ");
  //Serial.print(pulseCount);
  //Serial.println("");
 }
