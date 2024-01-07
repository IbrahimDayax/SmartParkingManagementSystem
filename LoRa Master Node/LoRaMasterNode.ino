#include <SPI.h>              // include libraries
#include <LoRa.h>
#include <Adafruit_GFX.h>
#include <stdio.h>
#include <Preferences.h>
#include <WiFi.h>
#include <ESPAsyncWebSrv.h>

#define ss 5
#define rst 14
#define dio0 2

byte MasterNode = 0xFF;
byte Node1 = 0xBB;
byte Node2 = 0xCC;
byte Node3 = 0xDD;

String SenderNode = "";
String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages
String incoming = "";
// Tracks the time since last event fired
unsigned long previousMillis = 0;
unsigned long int previousSecs = 0;
unsigned long int currentSecs = 0;
unsigned long currentMillis = 0;
int interval = 1 ; // updated every 1 second
int seconds = 0;
int x;
int y;
int z;
int headingDegrees;
String message;

float magnitude;

float magsNode1[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
float magsNode2[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
float magsNode3[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

float refMagNode1 = 0;
float refMagNode2 = 0;
float refMagNode3 = 0;

float refMagsNode1[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
float refMagsNode2[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
float refMagsNode3[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int magCount1 = 0;
int magCount2 = 0;
int magCount3 = 0;

bool statusNode1 = false;
bool statusNode2 = false;
bool statusNode3 = false;

unsigned long refMagTime;

Preferences preferences;

// const char *ssid = "GalaxyA13";
// const char *password = "ekkc0904";
const char *ssid = "BlazingSpeed-TIME_AB4F";
const char *password = "ng96samM";

AsyncWebServer server(80);

unsigned long occupiedStartTimeNode2 = 0;  // Store the time when Node2 becomes occupied

// HTML, CSS, and JS code stored in program memory
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 20px;
      text-align: center;
      background-color: #f4f4f4;
    }

    h1 {
      color: #333;
    }

    .node {
      display: inline-block;
      margin: 10px;
      width: 80px;
      height: 80px;
      border-radius: 50%;
      line-height: 80px;
      font-size: 16px;
      color: #fff;
    }

    .unoccupied {
      background-color: green;
    }

    .occupied {
      background-color: red;
    }
  </style>
  <title>Parking Status</title>
</head>
<body>
  <h1>Parking Status</h1>
  <div id="Node1" class="node %STATUS_NODE_1%"></div>
  <div id="Node2" class="node %STATUS_NODE_2%"></div>
  <div id="Node3" class="node %STATUS_NODE_3%"></div>
  <!-- Add more nodes as needed -->
  <script>
    function updateNodeStatus(nodeId, isOccupied) {
      var nodeElement = document.getElementById(nodeId);
      if (isOccupied) {
        nodeElement.classList.remove('unoccupied');
        nodeElement.classList.add('occupied');
      } else {
        nodeElement.classList.remove('occupied');
        nodeElement.classList.add('unoccupied');
      }
    }

    function updateParkingStatus(statusNode1, statusNode2, statusNode3) {
      updateNodeStatus('Node1', statusNode1);
      updateNodeStatus('Node2', statusNode2);
      updateNodeStatus('Node3', statusNode3);
      // Add more nodes as needed
    }

    function fetchAndUpdate() {
      fetch('/status')  // Assuming you have a new route for fetching status
        .then(response => response.json())
        .then(data => updateParkingStatus(data.statusNode1, data.statusNode2, data.statusNode3))
        .catch(error => console.error('Error fetching status:', error));
    }

    window.onload = function () {
      fetchAndUpdate();
      setInterval(fetchAndUpdate, 1000);  // Update every 1 seconds (adjust as needed)
    };
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);                   // initialize serial	
  delay(500);

  while (!Serial);
  Serial.println("LoRa Master Node");

  LoRa.setPins(ss, rst, dio0);

  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Open the preferences for the application, 'my-app'
  preferences.begin("my-app", false);  
  //get the latest status of the parking slots from the flash memory
  //statusNode2 = preferences.getBool("statusNode2", false);
  //refMagNode2 = preferences.getFloat("refMagNode2", 0);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.print("Connected to WiFi with IP: ");
  Serial.println(WiFi.localIP());

  // Route for root
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String htmlString(index_html);
    htmlString.replace("%STATUS_NODE_1%", String(statusNode1));
    htmlString.replace("%STATUS_NODE_2%", String(statusNode2));
    htmlString.replace("%STATUS_NODE_3%", String(statusNode3));
    request->send_P(200, "text/html", htmlString.c_str());
  });

  // Route for status 
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{\"statusNode1\":" + String(statusNode1) +
                  ",\"statusNode2\":" + String(statusNode2) +
                  ",\"statusNode3\":" + String(statusNode3) + "}";
    request->send(200, "application/json", json);
  });

  server.onNotFound(notFound);
  server.begin();

  refMagTime = millis();
}
void loop() {
  currentMillis = millis();
  currentSecs = currentMillis / 1000;
  if ((unsigned long)(currentSecs - previousSecs) >= interval) {
    seconds = seconds + 1;
    if ( seconds >= 16 )
    { 
      seconds = 0;
    }
    if ( (seconds >= 1) && (seconds <= 5) )
    {
      // message = "10";
      // sendMessage(message, MasterNode, Node1);
      message = "20";
      sendMessage(message, MasterNode, Node2);
      // message = "30";
      // sendMessage(message, MasterNode, Node3);
    }
    if ( (seconds >= 6 ) && (seconds <= 10))
    {
      // message = "10";
      // sendMessage(message, MasterNode, Node1);
      message = "20";
      sendMessage(message, MasterNode, Node2);
      // message = "30";
      // sendMessage(message, MasterNode, Node3);
    }
    if ( (seconds >= 11 ) && (seconds <= 15))
    {
      // message = "10";
      // sendMessage(message, MasterNode, Node1);
      message = "20";
      sendMessage(message, MasterNode, Node2);
      // message = "30";
      // sendMessage(message, MasterNode, Node3);
    }
    previousSecs = currentSecs;
  }

  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
}
void sendMessage(String outgoing, byte MasterNode, byte otherNode) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(otherNode);              // add destination address
  LoRa.write(MasterNode);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}
void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return
  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  if ( sender == 0XBB )
    SenderNode = "Node1:";
  if ( sender == 0XCC )
    SenderNode = "Node2:";
  if ( sender == 0XDD )
    SenderNode = "Node3:";
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length
  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }
  if (incomingLength != incoming.length()) {   // check length for error
    //Serial.println("error: message length does not match length");
    ;
    return;                             // skip rest of function
  }
  
  // if message is for this device, or broadcast, print details:
  //Serial.println("Received from: 0x" + String(sender, HEX));
  //Serial.println("Sent to: 0x" + String(recipient, HEX));
  //Serial.println("Message ID: " + String(incomingMsgId));
  // Serial.println("Message length: " + String(incomingLength));
  // Serial.println("Message: " + incoming);
  //Serial.println("RSSI: " + String(LoRa.packetRssi()));
  // Serial.println("Snr: " + String(LoRa.packetSnr()));
  // Serial.println();
  if ( sender == 0XBB )
  {

  }
  if ( sender == 0XCC )
  {
    // String xvalue = getValue(incoming, ',', 0); // x
    // String yvalue = getValue(incoming, ',', 1); // y
    // String zvalue = getValue(incoming, ',', 2); // z
    // String DHeading = getValue(incoming, ',', 3); // heading degrees

    // x = xvalue.toInt();
    // y = yvalue.toInt();
    // z = zvalue.toInt();
    // headingDegrees = DHeading.toInt();

    // incoming = "";

    // Serial.println(SenderNode);
    // Serial.println(statusNode2);
    // magnitude = sqrt(x * x + y * y + z * z);
    // Serial.print("Magnitude: "); Serial.println(magnitude);

    // if (magCount2 == 15) {
    //   if (isCarPresent(magsNode2, magCount2, refMagNode2)) {
    //     Serial.println("Car Present at Node 2");
    //     statusNode2 = true;
    //     occupiedStartTimeNode2 = millis();
    //   } else {
    //     Serial.println("Car Not Present at Node 2");
    //     statusNode2 = false;
    //   }
    //   // Resetting the array elements to 0
    //   for (int i = 0; i < 15; i++) {
    //     magsNode2[i] = 0;
    //   }
    //   magCount2 = 0;
    // } else if (magCount2 == 5) {
    //   // Check if 5 seconds have passed since the last reference value calculation
    //   if (!statusNode2 && (millis() - refMagTime > 5000)) {
    //     refMagNode2 = getRefMag(magsNode2, magCount2);
    //     Serial.print("Reference Magnitude: ");
    //     Serial.println(refMagNode2);
    //     preferences.putFloat("refMagNode2", refMagNode2);
    //     refMagTime = millis();
    //   }
    //   magCount2++;
    // } else {
    //   magsNode2[magCount2] = magnitude;
    //   magCount2++;
    // }

    // //store the status of the node/slot in permanent flash memory
    // preferences.putBool("statusNode2", statusNode2);

    String status = getValue(incoming, ',', 0);
    incoming = "";
    
    //just for testing purposes
    Serial.print("From Node2 received status: ");
    Serial.println(status);

    if (status == "0") {
      statusNode2 = false;
    } 
    
    if (status == "1") {
      statusNode2 = true;
    }

    Serial.print("statusNode2: ");
    Serial.println(statusNode2);

    //notify the node you recieved data
    // if (status == "true" || status == "1") {
    //   sendMessage("20", MasterNode, Node2);
    //   Serial.println("Car present at Node 2");
    // } else if (status == "false" || status == "0") {
    //   sendMessage("20", MasterNode, Node2);
    //   Serial.println("Car not present at Node 2");
    // } else {
    //   Serial.print("Wrong message from Node2: ");
    //   Serial.println(status);
    // }

    //store the status of the node/slot in permanent flash memory
    //preferences.putBool("statusNode2", statusNode2);

  }
    if ( sender == 0XDD )
  {

  }
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

String millisToMinutesAndSeconds(long millis) {
  long minutes = millis / 60000;
  long seconds = (millis % 60000) / 1000;
  return String(minutes) + "m " + String(seconds) + "s";
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}
