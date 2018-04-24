#include <Adafruit_NeoPixel.h>
#include <SPI.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#define PubNub_BASE_CLIENT WiFiClient
#include <PubNub.h>
#include "secrets.h"

ESP8266WiFiMulti WiFiMulti;

const static char channel[] = "leds"; // channel to use

char message[256];

volatile int pingNow;
unsigned long oldTime;

#define LED_PIN 13

Adafruit_NeoPixel handleLEDs = Adafruit_NeoPixel(8, LED_PIN, NEO_GRB + NEO_KHZ800);

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
}

void loop() {

  ping();


  colorWipe(handleLEDs.Color(255, 0, 0), 50);

  pollPubNub();

}

void pollPubNub() {
  WiFiClient *client;
  Serial.println("waiting for a message (subscribe)");

  PubSubClient *sclient = PubNub.subscribe(channel);
  if (sclient != 0) {
    while (sclient->wait_for_data(10)) {
        Serial.write(sclient->read());
    }
    sclient->stop();
  }

  /*
  client = PubNub.subscribe(channel);
  if (!client) {
    Serial.println("subscription error");
    delay(1000);
    return;
  } else {
    Serial.println("Message recieved");
    client->stop();
  }
  */
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

void createMessage(char* s, int usage) {
  sprintf(s, "{\"usage\": \"%d\",\"source\": \"rfid\",\"type\": \"counter\",\"uptime\": \"%d\"}", usage, millis() / 1000);
}

void createPing(char* s) {
  sprintf(s, "{\"source\": \"rfid\",\"type\": \"ping\",\"uptime\": \"%d\"}", millis() / 1000);
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

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<handleLEDs.numPixels(); i++) {
    handleLEDs.setPixelColor(i, c);
    handleLEDs.show();
    delay(wait);
  }
}

void colorPourProgress(uint16_t prog) {
  //prog should be between 0-100 (%)
  handleLEDs.numPixels();
  //handleLEDs.setPixelColor(i, handleLEDs.Color(102, 51, 0)); 
}

