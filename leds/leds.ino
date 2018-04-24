#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#define PubNub_BASE_CLIENT WiFiClient
#define PUBNUB_DEFINE_STRSPN_AND_STRNCASECMP
#include <PubNub.h>
#include <aJSON.h>

#define LED_PIN 13

#define PN_CHANNEL "Test"

Adafruit_NeoPixel handleLEDs = Adafruit_NeoPixel(8, LED_PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.


void setup() {
  Serial.begin(115200);
  Serial.print("Wait for WiFi... ");
  WiFi.begin("***", "****");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("");
  Serial.println("Configure PubNub"); 
  PubNub.begin("****", "******");

  Serial.println("Initialize Handle LED"); 
  initLedStrip(&handleLEDs);
}

void initLedStrip(Adafruit_NeoPixel *leds) {
  Serial.println("Initialize Handle LED"); 
  leds->begin();
  leds->show();
}

void loop() {
  colorWipe(handleLEDs.Color(255, 0, 0), 50);
  pollPubNub();

  if (WiFi.status() != WL_CONNECTED) {
     Serial.println("Wifi Error");
  }
}

void pollPubNub() {
  WiFiClient *client;
  Serial.println("waiting for a message (subscribe)");
  client = PubNub.subscribe(PN_CHANNEL);
  if (!client) {
    Serial.println("subscription error");
    delay(1000);
    return;
  } else {
    Serial.println("Message recieved");
    aJsonClientStream stream(client);
    Serial.println("Create stream");
    aJsonObject *msg = aJson.parse(&stream);
    Serial.println("Parsing");
    client->stop();
    if (!msg) {
      Serial.println("parse error");
      delay(1000);
      return;
    } else {
      dumpMessage(Serial, msg);
    }
    aJson.deleteItem(msg);
  }
}

void dumpMessage(Stream &s, aJsonObject *msg) {
  Serial.println("dumpMessage");
  colorWipe(handleLEDs.Color(255, 0, 0), 50); // Red
  Serial.println("dumpMessage");
  aJsonObject *name = aJson.getObjectItem(msg, "text");
  Serial.println("check");
  if (name == NULL) {
    Serial.println("unable to parse message");
    return;
  } else {
    Serial.println("parsed name");
    Serial.println(name->valuestring);
    Serial.println("parsed name ddd");
  }
Serial.println("print");
  char* string = aJson.print(msg);
  if (string != NULL) {
    Serial.println(string);
    free(string);
  } 
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


