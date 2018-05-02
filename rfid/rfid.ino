#include <SPI.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#define PubNub_BASE_CLIENT WiFiClient
#include <PubNub.h>
#include "secrets.h"

#include <MFRC522.h>
#define RST_PIN         5
#define SS_PIN          4

ESP8266WiFiMulti WiFiMulti;

const static char channel[] = "rfid"; // channel to use

char message[256];

volatile int pingNow;
unsigned long oldTime;
unsigned long feedbackTimer;
int ledStatus;
int feedback;

byte readCard[4];
MFRC522 mfrc522(SS_PIN, RST_PIN);

#define PN_CHANNEL "Test"

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
  
  SPI.begin();

  // Init the rfid reader
  mfrc522.PCD_Init();
  mfrc522.PCD_DumpVersionToSerial();
  delay(500);
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

  publishId();

}

int publishId() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return 0;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return 0;
  }
  int len = mfrc522.uid.size;
  char buffer[(len*2) + 1];
  for (byte i = 0; i < len; i++)
  {
     byte nib1 = (mfrc522.uid.uidByte[i] >> 4) & 0x0F;
     byte nib2 = (mfrc522.uid.uidByte[i] >> 0) & 0x0F;
     buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
     buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
  }
  buffer[len*2] = '\0';
  mfrc522.PICC_HaltA(); 
  Serial.println();
  Serial.print("Message : ");
  Serial.print(buffer);
  createMessage(message, buffer);
  publish(message);
  feedback = 5;
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

void createMessage(char* s, char* rfid) {
  sprintf(s, "{\"source\": \"rfid\",\"type\": \"event\",\"rfid\": \"%s\"}", rfid);
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

