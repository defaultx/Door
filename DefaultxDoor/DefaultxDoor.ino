#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <Adafruit_NFCShield_I2C.h>

#define IRQ   (2)
#define RESET (3)  // Not connected by default on the NFC Shield
#define NOTE_C6 1047

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0x98, 0x4F, 0xEE, 0x05, 0xAA, 0x88 };
IPAddress ip(192,168,1,100);
int port = 8088;
uint8_t keyID[] = {130,23,234,125};  // keyid  //0x82 0x17 0xEA 0x7D

Adafruit_NFCShield_I2C nfc(IRQ, RESET);

String message = "";
boolean alreadyConnected = false; // whether or not the client was connected previously
String key = "";
boolean locked = true;
String date,time_n;
String ex_date = "04/25/16";
String ex_time = "01:03:30";

// notes in the melody: 
int melody[] = {NOTE_C6, NOTE_C6, NOTE_C6, NOTE_C6,NOTE_C6, NOTE_C6, NOTE_C6, NOTE_C6};
// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {4, 4, 4, 4, 4, 4, 4, 4};

// Initialize the Ethernet server library
// with the IP address and port you want to use
EthernetServer server(port);
EthernetClient client;

// Pin 13 has an LED connected on most Arduino boards.
// give it a name:
int led = 13;
int lock = 7; //pin for the solonoid
int buzz = 8;
int x;
char buf[20];

void setup() {
  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);
  pinMode(lock, OUTPUT);
  // Open serial communications and wait for port to open:
  //Serial.begin(9600);
  Serial.begin(115200);
   while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  
 system("date 042501032016"); //sets the date & time mmddhhmmYY
  
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  nfc.begin();

   uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  nfc.setPassiveActivationRetries(0xFF);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
    
  Serial.println("Waiting for an ISO14443A card");
}


void loop() {
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  system("date '+%H:%M:%S %D' > /home/root/time.txt");  //get current time in the format- hours:minutes:secs //and save in text file time.txt located in /home/root
  
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

  if (success && locked) {
      getTime();
      Serial.println("Found a card!");
      Serial.print("UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
      Serial.print("UID Value: ");
      for (uint8_t i=0; i < uidLength; i++) 
      {
        Serial.print(" 0x");Serial.print(uid[i], HEX); 
      }
      Serial.println("");

      if(memcmp(uid, keyID, 4) == 0){
      Serial.println("Key Accepted...");
      Serial.println("Door Unlocked!");
      digitalWrite(lock, HIGH); //turn the lock pin to 5v
      locked = false;
    }
      // Wait 1 second before continuing
      delay(1000);
    }

  //*******for debug*******
  if(!locked){
    // Wait 10 second before continuing
      //delay(10000);
      buzzer();
       //for (x = 0; x < 15; x++){
     // digitalWrite(led, HIGH);
     // delay(500);
     // digitalWrite(led, LOW);
     // delay(500);
    //}
      locked = true; //lock the door after 10 sec or blin leds
      Serial.println("Door Locked!");
      Serial.println("");
  }

  if(ex_date != date && ex_time != time_n)
  {
    Serial.println("Client stay expired...");
    Serial.print("Key Changed:");
    for (uint8_t i=0; i < 4; i++) 
      {
        uid[i] += 10;
        Serial.print(" 0x");Serial.print(uid[i], HEX); 
      }
      Serial.println("");
  }
  client = server.available();
  if (client) {
    if (client.available()) {
      char c = client.read();
      if (c != '\n') {
        message += c;
      }
      else {
        Serial.println("Received message: " + message);
        sendMessage("Got it");//message);
        checkMessage();
        message = "";
      }
    }
  }
}


void checkMessage() {
  
  if (getValue(message, ':', 0) == "key") {
    Serial.print("Split: ");
    Serial.println(getValue(message, ':', 0));
    key = getValue(message, ':', 1);
    Serial.print("Key from server: ");
    Serial.println(key);
  }

  if (message == "on") {
    for (x = 0; x < 10; x++)
    {
      digitalWrite(led, HIGH);
      delay(500);
      digitalWrite(led, LOW);
      delay(500);
    }
  }

  if (message == "off") {
    digitalWrite(led, LOW);
  }
  if (message == "end") {
    // close the connection:
          sendMessage("Closing Connection");
          delay(1000);
          sendMessage("END");
          client.stop();
          Serial.println("Client Disonnected!");
  }
  if (message == "clean") {
    Serial.write(27);       // ESC command
    Serial.print("[2J");    // clear screen command
    Serial.write(27);
    Serial.print("[H");     // cursor to home command
  }
}

void sendMessage(String toSend) {
  if (client) {
    client.println(toSend + '\n');
    client.flush();
    //Serial.println("Send message: " + toSend);
  }
  else {
    Serial.println("Could not send message; Not connected.");
  }
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1  };
  int maxIndex = data.length()-1;
  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
      found++;
      strIndex[0] = strIndex[1]+1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
   }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void buzzer() {
  for (int thisNote = 0; thisNote < 8; thisNote++) {

    // to calculate the note duration, take one second
    // divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(buzz, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 4.00;
    digitalWrite(led, HIGH);
    delay(pauseBetweenNotes);
      digitalWrite(led, LOW);
    // stop the tone playing:
    noTone(8);
  }
}

void getTime() {
//system("date ' %H:%M:%S' > /home/root/time.txt"); //get current time in the format- hours:minutes:secs
// and save in text file time.txt located in /home/root
FILE *fp;
fp = fopen("/home/root/time.txt", "r");
fgets(buf, 20, fp);
fclose(fp);
Serial.print("The current time is ");
Serial.println(buf);
date = getValue(String(buf), ' ', 1);
time_n = getValue(String(buf), ' ', 0);
delay(100);
}
