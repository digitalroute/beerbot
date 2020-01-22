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

const static char pubNubId[] = "rfid";
const static char channelSend[] = "rfid";
const static char channelPing[] = "ping";

char message[256];

volatile int aliveNow;
unsigned long oldTime;
unsigned long feedbackTimer;
int ledStatus;
int feedback;

byte readCard[4];
MFRC522 mfrc522(SS_PIN, RST_PIN);

#define PN_CHANNEL "Test"

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
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  delay(500);

  generateId();

  PubNub.begin(SECRET_PUBKEY, SECRET_SUBKEY);
  PubNub.set_uuid(pubNubId);
  Serial.println("PubNub set up");

  oldTime = 0;
  feedbackTimer = 0;
  aliveNow = 0;
  ledStatus = 0;
  feedback = 0;
  sendBoot();
  
  SPI.begin();

  // Init the rfid reader
  mfrc522.PCD_Init();
  mfrc522.PCD_DumpVersionToSerial();
  delay(500);
}

void loop() {
  if((millis() - oldTime) > 1000) {
    oldTime = millis();

    sendAlive();
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
  publish(message, channelSend);
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
  sprintf(s, "{\"source\": \"%s\",\"type\": \"alive\",\"uptime\": \"%d\",\"id\": \"%s\"}", pubNubId, millis() / 1000, idString);
}

void createMessage(char* s, char* rfid) {
  sprintf(s, "{\"source\": \"%s\",\"type\": \"event\",\"rfid\": \"%s\",\"id\": \"%s\"}", pubNubId, rfid, idString);
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
