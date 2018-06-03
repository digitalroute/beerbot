#include <Adafruit_NeoPixel.h>
#include <SPI.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#define PubNub_BASE_CLIENT WiFiClient
#include <PubNub.h>
#include "secrets.h"
#include "animation.h"
#include <string.h>
ESP8266WiFiMulti WiFiMulti;

const static char channel[] = "leds"; // channel to use

#define BUF_LEN 256
char message[256];
char receiveBuffer[BUF_LEN];
char* lastReceived;

volatile int pingNow;
unsigned long oldTime;
unsigned int statusLed = LOW;

#define LED_PIN_ONBOARD 2
#define LED_PIN_HANDLE 13
#define LED_PIN_WINDOW 12

void HandleComplete();
void WindowComplete();

NeoPatterns handleLEDs(7, LED_PIN_HANDLE, NEO_GRB + NEO_KHZ800, &HandleComplete);
NeoPatterns windowLEDs(8, LED_PIN_WINDOW, NEO_GRB + NEO_KHZ800, &WindowComplete);

void setup() {
  Serial.begin(115200);
  delay(10);
  oldTime = millis();

  // We start by connecting to a WiFi network
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(SECRET_SSID, SECRET_SSID_PASSWORD);

  Serial.println();
  Serial.println();
  Serial.print("Wait for WiFi... ");

  while(WiFiMulti.run() != WL_CONNECTED) {
    updateStatusLed();
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

  cli();
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  timer1_write(600000); //120000 us
  sei();

  lastReceived = "red";
}

int flipCount = 0;
long deadline = 0;

void onTimerISR() {

  updateStatusLed();
  handleLEDs.Update();
  windowLEDs.Update();
  timer1_write(600000);
}

void loop() {

  doLeds();

  ping();

  pollPubNub();
}

void doLeds() {
  if(strstr(lastReceived, "blue")) {
    Serial.println("turn blue");
    handleLEDs.TheaterChase(handleLEDs.Color(0,0,255), handleLEDs.Color(0,0,0), 300);
    windowLEDs.TheaterChase(windowLEDs.Color(0,0,255), windowLEDs.Color(0,0,255), 300);
  } else if(strstr(lastReceived, "red")) {
    Serial.println("turn red");
    handleLEDs.TheaterChase(handleLEDs.Color(255,0,0), handleLEDs.Color(255,0,0), 300);
    windowLEDs.Scanner(windowLEDs.Color(255,0,0), 300);
  } else {
    Serial.println("nothing");
  }
  Serial.println("exit doleds");
}

void updateStatusLed() {
  unsigned long now = millis();
  if(now - oldTime > 500) {
    statusLed = !statusLed;
    oldTime = now;
  }
  digitalWrite(LED_PIN_ONBOARD, statusLed);
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
      if(i < BUF_LEN - 1) {
        receiveBuffer[i] = achar;
      }
      i++;
    }
    receiveBuffer[max(i+1, BUF_LEN - 1)] = 0;
    lastReceived = receiveBuffer;
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

void WindowComplete() {};
void HandleComplete() {};

