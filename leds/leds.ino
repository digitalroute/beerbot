#include <Adafruit_NeoPixel.h>
#include <SPI.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#define PubNub_BASE_CLIENT WiFiClient
#include <PubNub.h>
#include "secrets.h"
#include <string.h>
ESP8266WiFiMulti WiFiMulti;

const static char channel[] = "leds"; // channel to use

char message[256];
char lastReceived[256];

volatile int pingNow;
unsigned long oldTime;

#define LED_PIN_HANDLE 13
#define LED_PIN_WINDOW 12

Adafruit_NeoPixel handleLEDs = Adafruit_NeoPixel(8, LED_PIN_HANDLE, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel windowLEDs = Adafruit_NeoPixel(8, LED_PIN_WINDOW, NEO_GRB + NEO_KHZ800);

void setup() {
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
  windowLEDs.begin();
  windowLEDs.show();
  handleLEDs.begin();
  handleLEDs.show();
}

int flipCount = 0;
long deadline = 0;

void loop() {

  ping();

  pollPubNub();

  doLeds();
}

void doLeds() {
  if(strstr(lastReceived, "blue")) {
    Serial.println("turn blue");
    colorWipeHandle(handleLEDs.Color(0,0,255));
    colorWipeWindow(windowLEDs.Color(0,0,255));
  } else if(strstr(lastReceived, "red")) {
    Serial.println("turn red");
    colorWipeHandle(handleLEDs.Color(255,0,0));
    colorWipeWindow(windowLEDs.Color(255,0,0));
  } else {
    Serial.println("nothing");
  }
  Serial.println("exit doleds");
}

void colorWipeHandle(uint32_t c) {
  
  for(uint16_t i=0; i<handleLEDs.numPixels(); i++) {
    handleLEDs.setPixelColor(i, c);
  }
  handleLEDs.show();
}

void colorWipeWindow(uint32_t c) {
  
  for(uint16_t i=0; i<windowLEDs.numPixels(); i++) {
    windowLEDs.setPixelColor(i, c);
  }
  windowLEDs.show();
}

void pollPubNub() {
  WiFiClient *client;
  Serial.println("waiting for a message (subscribe)");

  PubSubClient *sclient = PubNub.subscribe(channel);
  if (sclient != 0) {
    int i = 0;
    while (sclient->wait_for_data(10)) {
      char achar = sclient->read();
      Serial.write(achar);
      lastReceived[i] = achar;
      i++;
    }
    lastReceived[i+1] = 0;
    sclient->stop();
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

void createPing(char* s) {
  sprintf(s, "{\"source\": \"leds\",\"type\": \"ping\",\"uptime\": \"%d\"}", millis() / 1000);
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

