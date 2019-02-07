// #include <SPI.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#define PubNub_BASE_CLIENT WiFiClient
#include <PubNub.h>
#include "secrets.h"
#include <string.h>
ESP8266WiFiMulti WiFiMulti;

#define VALVE_PIN_OPEN 12
#define VALVE_PIN_CLOSE 13
#define LED_PIN_OPEN 5
#define LED_PIN_CLOSE 4

#define DEFAULT_LED_FREQUENCY 25
#define OPEN_CLOSE_LED_FREQUENCY 5
#define BLINK_LED_FREQUENCY 2

const static char channel[] = "valve"; // channel to use

#define BUF_LEN 256
char message[256];
char receiveBuffer[BUF_LEN];
char* lastReceived = "open";

int ledStatus = 0;
int ledFrequencyCounter = 0;
int ledFrequency = DEFAULT_LED_FREQUENCY; // wait 10 ticks to toggle led

void setup() {
  Serial.begin(115200);
  delay(10);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(VALVE_PIN_OPEN, OUTPUT);
  pinMode(VALVE_PIN_CLOSE, OUTPUT);
  pinMode(LED_PIN_OPEN, OUTPUT);
  pinMode(LED_PIN_CLOSE, OUTPUT);

  digitalWrite(VALVE_PIN_OPEN, LOW);
  digitalWrite(VALVE_PIN_CLOSE, LOW);
  digitalWrite(LED_PIN_OPEN, LOW);
  digitalWrite(LED_PIN_CLOSE, LOW);
  
  // We start by connecting to a WiFi network
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(SECRET_SSID, SECRET_SSID_PASSWORD);

  Serial.println();
  Serial.println();
  Serial.print("Wait for WiFi... ");

  while(WiFiMulti.run() != WL_CONNECTED) {
    toggleLed();
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

  sendBootMessage();

  cli();
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  timer1_write(100000); //120000 us
  sei();
}

void onTimerISR() {
  if (ledFrequencyCounter > ledFrequency) {
    toggleLed();
    ledFrequencyCounter = 0;
  } else {
    ledFrequencyCounter++;
  }
  timer1_write(100000);
}

void loop() {
  pollPubNub();
  handleValve();
}

void handleValve() {
  Serial.println("handleValve called");
  Serial.print("lastReceived: ");
  Serial.println(lastReceived);
  if(strstr(lastReceived, "open")) {
    openValve();
  } else if(strstr(lastReceived, "close")) {
    closeValve();
  } else {
    // blinkLed(5);
    Serial.println("Command was not open or close");
  }
}

void openValve() {
  Serial.println("Opening valve");
  digitalWrite(LED_PIN_OPEN, LOW);
  digitalWrite(LED_PIN_CLOSE, LOW);
  digitalWrite(VALVE_PIN_CLOSE, LOW);
  digitalWrite(VALVE_PIN_OPEN, HIGH);
  ledWait(5);
  digitalWrite(VALVE_PIN_OPEN, LOW);
  digitalWrite(LED_PIN_OPEN, HIGH);
  digitalWrite(LED_PIN_CLOSE, LOW);
  Serial.println("Opened");
}

void closeValve() {
  Serial.println("Closing valve");
  digitalWrite(LED_PIN_OPEN, LOW);
  digitalWrite(LED_PIN_CLOSE, LOW);
  digitalWrite(VALVE_PIN_OPEN, LOW);
  digitalWrite(VALVE_PIN_CLOSE, HIGH);
  ledWait(5);
  digitalWrite(VALVE_PIN_CLOSE, LOW);
  digitalWrite(LED_PIN_OPEN, LOW);
  digitalWrite(LED_PIN_CLOSE, HIGH);
  Serial.println("Closed");
}

void blinkLed(int n) {
  ledFrequency = BLINK_LED_FREQUENCY;
  delay(n * 100);
  ledFrequency = DEFAULT_LED_FREQUENCY;
}

void ledWait(int seconds) {
  ledFrequency = OPEN_CLOSE_LED_FREQUENCY;
  delay(seconds * 1000);
  ledFrequency = DEFAULT_LED_FREQUENCY;
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
    receiveBuffer[i] = 0;
    Serial.println();
    lastReceived = receiveBuffer;
    sclient->stop();
  }
}

void ping() {
  Serial.println("Sending ping");
  createPing(message);
  publish(message);
}

void createPing(char* s) {
  sprintf(s, "{\"source\": \"valve\",\"type\": \"ping\",\"uptime\": \"%d\"}", millis() / 1000);
}

void sendBootMessage() {
  Serial.println("Sending boot message");
  createBootMessage(message);
  publish(message);
}

void createBootMessage(char* s) {
  sprintf(s, "{\"source\": \"valve\",\"type\": \"boot\",\"uptime\": \"%d\"}", millis() / 1000);
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
