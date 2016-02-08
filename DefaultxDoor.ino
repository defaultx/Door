#include <SPI.h>
#include <Ethernet.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:

byte mac[] = { 0x98, 0x4F, 0xEE, 0x05, 0xAA, 0x88 };
IPAddress ip(192,168,1,100);
int port = 8088;

String message = "";
boolean alreadyConnected = false; // whether or not the client was connected previously
String key = "";

// Initialize the Ethernet server library
// with the IP address and port you want to use
EthernetServer server(port);
EthernetClient client;

// Pin 13 has an LED connected on most Arduino boards.
// give it a name:
int led = 13;

int x;

void setup() {
  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
   while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}


void loop() {
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
