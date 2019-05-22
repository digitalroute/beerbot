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

const static char pubNubId[] = "leds";
const static char channelListen[] = "leds";
const static char channelPing[] = "ping";

#define BUF_LEN 256
char message[256];
char receiveBuffer[BUF_LEN];
char* lastReceived;

unsigned long oldTime;
unsigned int statusLed = LOW;

#define LED_PIN_HANDLE 13
#define LED_PIN_WINDOW 12

#define NUMBER_OF_LEDS_HANDLE 7
#define NUMBER_OF_LEDS_WINDOW 7

#define TIMER_TICKS 10000

void HandleComplete();
void WindowComplete();

NeoPatterns handleLEDs(NUMBER_OF_LEDS_HANDLE, LED_PIN_HANDLE, NEO_GRB + NEO_KHZ800, &HandleComplete);
NeoPatterns windowLEDs(NUMBER_OF_LEDS_WINDOW, LED_PIN_WINDOW, NEO_GRB + NEO_KHZ800, &WindowComplete);

void setup() {
  Serial.begin(115200);
  delay(10);
  oldTime = millis();

  pinMode(LED_BUILTIN, OUTPUT);

  randomSleep();
  
  // We start by connecting to a WiFi network
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(SECRET_SSID, SECRET_SSID_PASSWORD);

  Serial.println();
  Serial.println();
  Serial.print("Wait for WiFi... ");

  while(WiFiMulti.run() != WL_CONNECTED) {
    toggleStatusLed();
    Serial.print(".");
    delay(100);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  delay(500);

  PubNub.begin(SECRET_PUBKEY, SECRET_SUBKEY);
  PubNub.set_uuid(pubNubId);
  Serial.println("PubNub set up");

  oldTime = 0;
  windowLEDs.begin();
  windowLEDs.show();
  handleLEDs.begin();
  handleLEDs.show();

  cli();
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  timer1_write(TIMER_TICKS);
  sei();

  lastReceived = "bootcolor";

  sendBoot();
}

void onTimerISR() {
  updateStatusLed();
  handleLEDs.Update();
  windowLEDs.Update();
  timer1_write(TIMER_TICKS);
}

void loop() {
  doLeds();
  pollPubNub();
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
    toggleStatusLed();
  }

  Serial.println();
}

void doLeds() {
  Serial.print("entering doLeds with: ");
  Serial.print(lastReceived);
  Serial.println("");
  if(strstr(lastReceived, "scanner")) {
    doScanner();
  } else if(strstr(lastReceived, "theaterchase")) {
    doTheaterChase();
  } else if(strstr(lastReceived, "rainbow")) {
    doRainbow();
  } else if(strstr(lastReceived, "bootcolor")) {
    Serial.println("turn bootcolor");
    handleLEDs.RainbowCycle(5);
    windowLEDs.RainbowCycle(5);
  } else if(strstr(lastReceived, "ping")) {
    Serial.println("ping");
    sendPong();
  } else {
    Serial.println("nothing");
  }
  Serial.println("exit doleds");
}

void toggleStatusLed() {
  statusLed = !statusLed;
  digitalWrite(LED_BUILTIN, statusLed);
}

void updateStatusLed() {
  unsigned long now = millis();
  if(now - oldTime > 500) {
    toggleStatusLed();
    oldTime = now;
  }
}

void pollPubNub() {
  WiFiClient *client;
  Serial.println("waiting for a message (subscribe)");

  PubSubClient *sclient = PubNub.subscribe(channelListen);
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
  Serial.println("");
}

void sendBoot() {
  Serial.println("Sending boot");
  createBootMessage(message);
  publish(message);
}

void createBootMessage(char* s) {
  sprintf(s, "{\"source\": \"%s\",\"type\": \"boot\",\"uptime\": \"%d\"}", pubNubId, millis() / 1000);
}

void sendPong() {
  Serial.println("Sending pong");
  createPong(message);
  publish(message);
}

void createPong(char* s) {
  sprintf(s, "{\"source\": \"%s\",\"type\": \"pong\",\"uptime\": \"%d\"}", pubNubId, millis() / 1000);
}

void publish(char* msg) {
  WiFiClient *client;
  
  client = PubNub.publish(channelPing, msg);

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

// ;scanner;window;i;r;g;b;
void doScanner() {
  char program[32] = {0};
  char placement[32] = {0};
  unsigned long i = 0;
  int r = 0;
  int g = 0;
  int b = 0;

  char *strtokIndx; // this is used by strtok() as an index

  strtokIndx = strtok(lastReceived,";"); // part before ;scanner
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  strtokIndx = strtok(NULL, ";");        // ;scanner;
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  strcpy(program, strtokIndx);
  strtokIndx = strtok(NULL, ";");        // ;window;
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  strcpy(placement, strtokIndx);
  strtokIndx = strtok(NULL, ";");        // ;r;
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  i = atol(strtokIndx);
  strtokIndx = strtok(NULL, ";");        // ;r;
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  r = atoi(strtokIndx);
  strtokIndx = strtok(NULL, ";");        // ;g;
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  g = atoi(strtokIndx);
  strtokIndx = strtok(NULL, ";");        // ;b;
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  b = atoi(strtokIndx);
  
  Serial.print("program: ");
  Serial.println(program);
  Serial.print("placement: ");
  Serial.println(placement);
  Serial.print("i: ");
  Serial.println(i);
  Serial.print("r: ");
  Serial.println(r);
  Serial.print("g: ");
  Serial.println(g);
  Serial.print("b: ");
  Serial.println(b);

  if (strstr(placement, "handle")) {
    Serial.println("scanner: handle");
    handleLEDs.Scanner(handleLEDs.Color(r,g,b), i);
  } else if (strstr(placement, "window")) {
    Serial.println("scanner: window");
    windowLEDs.Scanner(windowLEDs.Color(r,g,b), i);
  } else {
    Serial.println("scanner: placement not found");      
  }
};


// ;scanner;window;i;r;g;b;
void doTheaterChase() {
  char program[32] = {0};
  char placement[32] = {0};
  unsigned long i = 0;
  int r_1 = 0;
  int g_1 = 0;
  int b_1 = 0;
  int r_2 = 0;
  int g_2 = 0;
  int b_2 = 0;

  char *strtokIndx; // this is used by strtok() as an index

  strtokIndx = strtok(lastReceived,";"); // part before ;scanner
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  strtokIndx = strtok(NULL, ";");        // ;scanner;
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  strcpy(program, strtokIndx);
  strtokIndx = strtok(NULL, ";");        // ;window;
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  strcpy(placement, strtokIndx);
  strtokIndx = strtok(NULL, ";");        // ;r;
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  i = atol(strtokIndx);
  strtokIndx = strtok(NULL, ";");        // ;r;
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  r_1 = atoi(strtokIndx);
  strtokIndx = strtok(NULL, ";");        // ;g;
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  g_1 = atoi(strtokIndx);
  strtokIndx = strtok(NULL, ";");        // ;b;
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  b_1 = atoi(strtokIndx);
  strtokIndx = strtok(NULL, ";");        // ;r;
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  r_2 = atoi(strtokIndx);
  strtokIndx = strtok(NULL, ";");        // ;g;
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  g_2 = atoi(strtokIndx);
  strtokIndx = strtok(NULL, ";");        // ;b;
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  b_2 = atoi(strtokIndx);

  Serial.print("program: ");
  Serial.println(program);
  Serial.print("placement: ");
  Serial.println(placement);
  Serial.print("i: ");
  Serial.println(i);
  Serial.print("r_1: ");
  Serial.println(r_1);
  Serial.print("g_1: ");
  Serial.println(g_1);
  Serial.print("b_1: ");
  Serial.println(b_1);
  Serial.print("r_2: ");
  Serial.println(r_2);
  Serial.print("g_2: ");
  Serial.println(g_2);
  Serial.print("b_2: ");
  Serial.println(b_2);

  if (strstr(placement, "handle")) {
    Serial.println("theaterchase: handle");
    handleLEDs.TheaterChase(handleLEDs.Color(r_1,g_1,b_1), handleLEDs.Color(r_2,g_2,b_2), i);
    // handleLEDs.Scanner(handleLEDs.Color(r,g,b), i);
  } else if (strstr(placement, "window")) {
    Serial.println("theaterchase: window");
    //windowLEDs.Scanner(windowLEDs.Color(r,g,b), i);
    windowLEDs.TheaterChase(windowLEDs.Color(r_1,g_1,b_1), windowLEDs.Color(r_2,g_2,b_2), i);
  } else {
    Serial.println("theaterchase: placement not found");      
  }
};


// ;scanner;window;i;r;g;b;
void doRainbow() {
  char program[32] = {0};
  char placement[32] = {0};
  unsigned long i = 0;

  char *strtokIndx; // this is used by strtok() as an index

  strtokIndx = strtok(lastReceived,";"); // part before ;scanner
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  strtokIndx = strtok(NULL, ";");        // ;scanner;
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  strcpy(program, strtokIndx);
  strtokIndx = strtok(NULL, ";");        // ;window;
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  strcpy(placement, strtokIndx);
  strtokIndx = strtok(NULL, ";");        // ;r;
  if (strtokIndx == NULL) { Serial.println("strtokIndx is null"); return; }
  i = atol(strtokIndx);

  Serial.print("program: ");
  Serial.println(program);
  Serial.print("placement: ");
  Serial.println(placement);
  Serial.print("i: ");
  Serial.println(i);

  if (strstr(placement, "handle")) {
    Serial.println("rainbow: handle");
    handleLEDs.RainbowCycle(i);
  } else if (strstr(placement, "window")) {
    Serial.println("rainbow: window");
    windowLEDs.RainbowCycle(i);
  } else {
    Serial.println("rainbow: placement not found");      
  }
};
