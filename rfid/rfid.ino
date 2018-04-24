#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#define PubNub_BASE_CLIENT WiFiClient
#define PUBNUB_DEFINE_STRSPN_AND_STRNCASECMP
#include <PubNub.h>
//#include <aJSON.h>

#define RST_PIN         5          // Configurable, see typical pin layout above
#define SS_PIN          4         // Configurable, see typical pin layout above

byte readCard[4];
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

#define PN_CHANNEL "Test"

void setup() {
	Serial.begin(115200);;		// Initialize serial communications with the PC
  Serial.print("Starting");
	while (!Serial);
	SPI.begin();
	mfrc522.PCD_Init();
	mfrc522.PCD_DumpVersionToSerial();
  delay(500);

  Serial.print("Wait for WiFi... ");
  WiFi.begin("*****", "*****");

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
  //Do pubnub init here......
}

void loop() {
  getID2();
	/*// Look for new cards
	if ( ! mfrc522.PICC_IsNewCardPresent()) {
		return;
	}

	// Select one of the cards
  Serial.println("Check card...");
	if ( ! mfrc522.PICC_ReadCardSerial()) {
		return;
	}

	// Dump debug info about the card; PICC_HaltA() is automatically called
 Serial.print("Dump card.");
	mfrc522.PICC_DumpToSerial(&(mfrc522.uid));*/
  
}

uint8_t getID2() {
    // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  Serial.print("UID tag :");
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  mfrc522.PICC_HaltA(); 
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
}

uint8_t getID() {
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  Serial.println(F("Scanned PICC's UID:"));
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
  }
  Serial.println("");
  String myString = String((char *)readCard);
  Serial.println(myString);
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}
