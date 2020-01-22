
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

const static char pubNubId[] = "flowmeter";
const static char channelSend[] = "flow";
const static char channelPing[] = "ping";

char message[256];

byte sensorPin = 13;  // input pin
//byte interruptPin = 2;
volatile int pulseCount = 0;
unsigned long totalCount = 0;
int sendZeroUsage = 0;

volatile int aliveNow;
unsigned long oldTime;
unsigned long feedbackTimer;
int ledStatus;
int feedback;

// idString set to = CONFIG_BOT_ID/mac address
char idString[32];

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.print("setup()");

  pinMode(LED_BUILTIN, OUTPUT);

  randomSleep();
  
  // We start by connecting to a WiFi network
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(SECRET_SSID, SECRET_SSID_PASSWORD);

  Serial.println();
  Serial.println();
  Serial.print("Wait for WiFi... ");

  while(WiFiMulti.run() != WL_CONNECTED) {
    toggleLed();
    Serial.print(".");
    delay(100);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  generateId();

  delay(500);

  PubNub.begin(SECRET_PUBKEY, SECRET_SUBKEY);
  Serial.println("PubNub set up");

  oldTime = 0;
  feedbackTimer = 0;
  aliveNow = 0;
  ledStatus = 0;
  feedback = 0;
  sendBoot();

  pinMode(sensorPin, INPUT_PULLUP);
  digitalWrite(sensorPin, HIGH);
  //attachInterrupt(interruptPin, pulseCounter, FALLING);
  attachInterrupt(digitalPinToInterrupt(sensorPin), pulseCounter, FALLING);
  Serial.println("All setup done");
}

void loop() {
  if((millis() - oldTime) > 1000) {
    oldTime = millis();

    sendAlive();
    if (feedback == 0) {
      toggleLed();
    }

    if (pulseCount > 0) {
      createMessage(message, pulseCount, totalCount);
      publish(message, channelSend);
      sendZeroUsage = 1;
      pulseCount = 0;
    } else {
      if (sendZeroUsage > 0) {
        createMessage(message, 0, totalCount);
        publish(message, channelSend);
        sendZeroUsage = 0;
      }
    }
  }

  if ((millis() - feedbackTimer) > 200 && feedback > 0) {
    toggleLed();
    feedback--;
  }

}

void generateId() {
  byte mac[6];

  WiFi.macAddress(mac);
  sprintf(idString, "%s/%02X%02X%02X%02X%02X%02X", CONFIG_BOT_ID, mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
  Serial.print("ID (mac address): ");
  Serial.println(idString);
}

void randomSleep() {
  int randomSleep = random(20);
  Serial.println();
  Serial.print("Will sleep for ");
  Serial.print(randomSleep);
  Serial.print(" seconds before starting WiFi: ");

  for (int i = 0; i < randomSleep; i++) {
    Serial.print(".");
    delay(1000);
    toggleLed();
  }

  Serial.println();
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

void sendBoot() {
  Serial.println("Sending boot");
  createBootMessage(message);
  publish(message, channelPing);
}

void createBootMessage(char* s) {
  sprintf(s, "{\"source\": \"%s\",\"type\": \"boot\",\"uptime\": \"%d\",\"id\": \"%s\"}", pubNubId, millis() / 1000, idString);
}

void sendAlive() {
  if (aliveNow >= 30) {
    Serial.println("Sending alive");
    createAlive(message);
    publish(message, channelPing);
    aliveNow = 0;
  }

  aliveNow++;
}

void createAlive(char* s) {
  sprintf(s, "{\"source\": \"%s\",\"total\": \"%d\",\"type\": \"alive\",\"uptime\": \"%d\",\"id\": \"%s\"}", pubNubId, totalCount, millis() / 1000, idString);
}

void createMessage(char* s, int usage, int total) {
  sprintf(s, "{\"source\": \"%s\",\"usage\": \"%d\",\"total\": \"%d\",\"type\": \"counter\",\"uptime\": \"%d\",\"id\": \"%s\"}", pubNubId, usage, total, millis() / 1000, idString);
}

void publish(char* msg, const char* chnl) {
  WiFiClient *client;
  
  client = PubNub.publish(chnl, msg);

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
