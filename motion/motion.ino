/*
 *  Read PIR motion sensor and publish to pubnub when motion is detected.
 *
 */
#include <SPI.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#define PubNub_BASE_CLIENT WiFiClient
#include <PubNub.h>
#include "secrets.h"

ESP8266WiFiMulti WiFiMulti;

const static char channel[] = "motion"; // channel to use

char message[256];

volatile int pingNow;
unsigned long oldTime;

int sensor = 5;              // the pin that the sensor is atteched to
int state = LOW;             // by default, no motion detected

void setup() {
  pinMode(sensor, INPUT);

  Serial.begin(115200);
  delay(10);

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
  pingNow = 10000; // make sure to send a ping first :)
}

void loop() {
  int latestRead = digitalRead(sensor);

  // Only process counters once per second
  if((millis() - oldTime) > 1000) {
    oldTime = millis();

    ping();

    Serial.println(latestRead, DEC);
  
    if (latestRead == HIGH) {
      delay(100);
  
      if (state == LOW) {
        Serial.println("Motion detected!"); 
        state = HIGH;
        createMessage(message, state);
        publish(message);
      }
    } 
    else {
      delay(200);
  
      if (state == HIGH) {
        Serial.println("Motion stopped!");
        state = LOW;
      }
    }
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

void createMessage(char* s, int state) {
  char* detectedState;
  if(state == HIGH) {
    detectedState = "motion-detected";
  } else {
    detectedState = "motion-stopped";
  }
  sprintf(s, "{\"source\": \"motion\",\"type\": \"%s\",\"uptime\": \"%d\"}", detectedState, millis() / 1000);
}

void createPing(char* s) {
  sprintf(s, "{\"source\": \"motion\",\"type\": \"ping\",\"uptime\": \"%d\"}", millis() / 1000);
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


