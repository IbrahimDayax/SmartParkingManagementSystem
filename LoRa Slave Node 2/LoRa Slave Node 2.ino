#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>

#define ss 10
#define rst 9
#define dio0 2

String outgoing;              // Outgoing message

byte msgCount = 0;            // Count of outgoing messages
byte MasterNode = 0xFF;
byte Node2 = 0xCC;

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
  pinMode(dio0, INPUT);
  pinMode(rst, OUTPUT);
  pinMode(ss, OUTPUT);

  /* Initialize the sensor */
  if (!mag.begin()) {
    /* There was a problem detecting the HMC5883 ... check your connections */
    Serial.println("Ooops, no HMC5883 detected ... Check your wiring!");
    while (1);
  }

  while (!Serial);

  Serial.println("LoRa Node2");

  LoRa.setPins(ss, rst, dio0);

  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {

  messageReceived = false;
  currentStatus = false;

  /* Get a new sensor event */
  sensors_event_t event;
  mag.getEvent(&event);

  // Get sensor values
  X = event.magnetic.x;
  Y = event.magnetic.y;
  Z = event.magnetic.z;


  //Delay to prevent sending messages too frequently and to limit effect of noisy values
  delay(50);

  //Calculate magnitude
  magnitude = sqrt(X * X + Y * Y + Z * Z);

  // Calculate heading
  heading = atan2(event.magnetic.y, event.magnetic.x);

  // Correct for magnetic declination
  declinationAngle = 0.018; // Your declination angle here for Malaysia
  heading += declinationAngle;

  // Correct for when signs are reversed
  if (heading < 0)
    heading += 2 * PI;

  // Check for wrap due to addition of declination
  if (heading > 2 * PI)
    heading -= 2 * PI;

  // Convert radians to degrees for readability
  headingDegrees = heading * 180 / M_PI;

  if (magCount == 15) {
    if (isCarPresent(magnitudes, magCount, refMagnitude)) {
      //Serial.println("Car Present at Node 2");
      //Serial.println("1");
      currentStatus = true;
      //occupiedStartTimeNode2 = millis();
    } else {
      //Serial.println("Car Not Present at Node 2");
      //Serial.println("0");
      currentStatus = false;
    }
    //delay(500);
    // Parse for a packet, and call onReceive with the result:
    onReceive(LoRa.parsePacket());
    // Resetting the array elements to 0
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
  if (packetSize == 0) return;          // If there's no packet, return

  // Read packet header bytes:
  int recipient = LoRa.read();          // Recipient address
  byte sender = LoRa.read();            // Sender address
  byte incomingMsgId = LoRa.read();     // Incoming message ID
  byte incomingLength = LoRa.read();    // Incoming message length

  String incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length()) {   // Check length for error
    // Serial.println("error: message length does not match length");
    ;
    return;                             // Skip rest of the function
  }

  // If the recipient isn't this device or broadcast,
  if (recipient != Node2 && recipient != MasterNode) {
    //Serial.println("This message is not for me.");
    ;
    return;                             // Skip rest of the function
  }

  Serial.println(incoming);
  int Val = getValue(incoming, ",", 0).toInt();
  oldStatus = (getValue(incoming, ",", 1) == "true" || getValue(incoming, ",", 1) == "1");
  if (Val == 20) {
    //myMessage = myMessage + X + "," + Y + "," + Z + "," + headingDegrees;
    myMessage = myMessage + currentStatus + ",";
    sendMessage(myMessage, MasterNode, Node2);
    myMessage = "";
    Serial.print("Transmitted status: ");
    Serial.println(currentStatus);
  }

  // if (Val == 20) {
  //   messageReceived = true;
  // }
}

void sendMessage(String outgoing, byte MasterNode, byte Node2) {
  LoRa.beginPacket();                   // Start packet
  LoRa.write(MasterNode);              // Add destination address
  LoRa.write(Node2);             // Add sender address
  LoRa.write(msgCount);                 // Add message ID
  LoRa.write(outgoing.length());        // Add payload length
  LoRa.print(outgoing);                 // Add payload
  LoRa.endPacket();                     // Finish packet and send it
  msgCount++;                           // Increment message ID
}

String getValue(String data, char separator, int index)
{
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
