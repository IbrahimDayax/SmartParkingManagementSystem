#include <SPI.h>
#include <RadioLib.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>

#define RADIO_NSS_PIN 10        // Pin connected to the NSS (Chip Select) of the LoRa module
#define RADIO_RST_PIN 9         // Pin connected to the reset of the LoRa module
#define RADIO_DIO0_PIN 2        // Pin connected to DIO0 of the LoRa module
#define RADIO_DIO1_PIN 3        // Pin connected to DIO1 of the LoRa module

Module module(RADIO_NSS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN, RADIO_DIO1_PIN);
SX1278 radio(&module);

byte msgCount = 0;            // Count of outgoing messages
byte MasterNode = 0xFF;
byte Node1 = 0xBB;

float X;
float Y;
float Z;
float heading;
float headingDegrees;
float declinationAngle;
float magnitude;
float magnitudes[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
float refMagnitude = 0;
int magCount = 0;
bool currentStatus = false;
bool oldStatus;
bool messageReceived = false;
unsigned long elapsedTime = 0;

String myMessage = "";
String incoming = "";

/* Create a unique ID for this sensor */
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);

void setup() {
  Serial.begin(9600);                   // Initialize serial

  if (!mag.begin()) {
    /* There was a problem detecting the HMC5883 ... check your connections */
    Serial.println("Ooops, no HMC5883 detected ... Check your wiring!");
    while (1);
  }

  while (!Serial);

  Serial.println("LoRa Node1");

  // Set LoRa module parameters
  radio.setFrequency(434.0);
  radio.setBandwidth(125000);
  radio.setSpreadingFactor(12);
  radio.setCodingRate(5);
  radio.setPreambleLength(8);
  radio.setSyncWord(0x12);

  // Check if LoRa module initialization is successful
  if (radio.begin() != 0 && radio.begin() != false) {
    Serial.println("LoRa init failed");
    Serial.println(radio.begin());
    while (1);
  }

  Serial.println("Radio initialized successfully");
}

void loop() {
  messageReceived = false;
  currentStatus = false;

  //Test to see if slave node can send data
  myMessage = "0";
  sendMessage(myMessage, MasterNode, Node1);
  Serial.print("Sent message: ");
  Serial.print(myMessage);
  Serial.println(" to master node");

  /* Get a new sensor event */
  sensors_event_t event;
  mag.getEvent(&event);

  // Get sensor values
  X = event.magnetic.x;
  Y = event.magnetic.y;
  Z = event.magnetic.z;

  // Delay to prevent sending messages too frequently and to limit the effect of noisy values
  delay(50);

  // Calculate magnitude
  magnitude = sqrt(X * X + Y * Y + Z * Z);

  // Calculate heading
  heading = atan2(event.magnetic.y, event.magnetic.x);

  // Correct for magnetic declination
  declinationAngle = 0.018; // Your declination angle here for Malaysia
  heading += declinationAngle;

  // Correct for when signs are reversed
  if (heading < 0)
    heading += 2 * PI;

  // Check for wrap due to the addition of declination
  if (heading > 2 * PI)
    heading -= 2 * PI;

  // Convert radians to degrees for readability
  headingDegrees = heading * 180 / M_PI;

  if (magCount == 15) {
    if (isCarPresent(magnitudes, magCount, refMagnitude)) {
      currentStatus = true;
    } else {
      currentStatus = false;
    }

    onReceive(radio.available());

    for (int i = 0; i < 15; i++) {
      magnitudes[i] = 0;
    }
    magCount = 0;
  } else if (magCount == 5) {
    if (currentStatus == false && refMagnitude == 0) {
      refMagnitude = getRefMag(magnitudes, magCount);
      Serial.print("Reference Magnitude: ");
      Serial.println(refMagnitude);
    }
    magCount++;
  } else {
    magnitudes[magCount] = magnitude;
    magCount++;
  }
}

void onReceive(int packetSize) {
  if (packetSize > 0) {
        int recipient = radio.read();
    byte sender = radio.read();
    byte incomingMsgId = radio.read();
    byte incomingLength = radio.read();

    Serial.print("Received packet. Recipient: ");
    Serial.print(recipient, HEX);
    Serial.print(", Sender: ");
    Serial.print(sender, HEX);
    Serial.print(", Msg ID: ");
    Serial.print(incomingMsgId);
    Serial.print(", Length: ");
    Serial.println(incomingLength);

    String incoming = "";

    while (radio.available()) {
      incoming += (char)radio.read();
    }

    Serial.print("Received data: ");
    Serial.println(incoming);

    if (incomingLength != incoming.length()) {
      Serial.println("Error: Message length mismatch!");
      return;
    }

    if (recipient != Node1 && recipient != MasterNode) {
      Serial.println("Error: Incorrect recipient address!");
      return;
    }

    Serial.println(incoming);
    int Val = getValue(incoming, ",", 0).toInt();
    oldStatus = (getValue(incoming, ",", 1) == "true" || getValue(incoming, ",", 1) == "1");

    if (Val == 100 && messageReceived == false) {
      myMessage = myMessage + currentStatus + " This is just extra stuff,";
      sendMessage(myMessage, MasterNode, Node1);
      Serial.print("Transmitted status: ");
      Serial.println(currentStatus);
      myMessage = "";
      Val = 0;
    }

    if (Val == 10) {
      messageReceived = true;
    }
  }
}

void sendMessage(String outgoing, byte MasterNode, byte Node1) {
  // Convert the String to a char array
  char msg[outgoing.length() + 1];
  outgoing.toCharArray(msg, outgoing.length() + 1);

  // Transmit the message
  radio.transmit(Node1, msg, outgoing.length() + 1);
  msgCount++;  // Increment message ID
}


String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

bool isCarPresent(float magnitudes[], int length, float referenceValue) {
  int count = 0;

  for (int i = 0; i < length; i++) {
    if (magnitudes[i] >= referenceValue + 7 || magnitudes[i] <= referenceValue - 7) {
      count++;
    } else {
      count = 0;
    }

    if (count >= 3) {
      return true;
    }
  }

  return false;
}

float getRefMag(float magnitudes[], int length) {
  float sum = 0.0;
  int count = 0;
  float mean = 0.0;
  float processedMagnitudes[length];
  int processedCount = 0;

  // Calculate the mean of non-zero values
  for (int i = 0; i < length; i++) {
    if (magnitudes[i] != 0) {
      sum += magnitudes[i];
      count++;
    }
  }

  if (count == 0) {
    return 0.0;  // If no non-zero values, return 0
  }

  mean = sum / count;

  // Remove values that deviate too much from the mean
  for (int i = 0; i < length; i++) {
    if (magnitudes[i] != 0 && abs(magnitudes[i] - mean) < 10) {
      processedMagnitudes[processedCount] = magnitudes[i];
      processedCount++;
    }
  }

  if (processedCount == 0) {
    return mean;  // Return mean if no values are within range
  }

  sum = 0;
  for (int i = 0; i < processedCount; i++) {
    sum += processedMagnitudes[i];
  }

  return sum / processedCount;  // Calculate the new average reference magnitude
}
