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
unsigned long int previoussecs = 0;
unsigned long int currentsecs = 0;
unsigned long currentMillis = 0;
int interval = 1 ; // updated every 1 second
int Secs = 0;
int x;
int y;
int z;
int headingdegrees;

struct Status {
  bool statusNode1;
  bool statusNode2;
  bool statusNode3;
};

Status current;

Preferences preferences;

// const char *ssid = "BlazingSpeed-TIME_AB4F";
// const char *password = "ng96samM";
const char *ssid = "GalaxyA13";
const char *password = "ekkc0904";

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
      setInterval(fetchAndUpdate, 5000);  // Update every 5 seconds (adjust as needed)
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
  current.statusNode1 = preferences.getBool("statusNode1", false);
  current.statusNode2 = preferences.getBool("statusNode2", false);
  current.statusNode3 = preferences.getBool("statusNode3", false);
  //refMagNode2 = preferences.getFloat("refMagNode2", 0);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.print("Connected to WiFi with IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String htmlString(index_html);
    htmlString.replace("%STATUS_NODE_1%", String(current.statusNode1));
    htmlString.replace("%STATUS_NODE_2%", String(current.statusNode2));
    htmlString.replace("%STATUS_NODE_3%", String(current.statusNode3));
    request->send_P(200, "text/html", htmlString.c_str());
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{\"statusNode1\":" + String(current.statusNode1) +
                  ",\"statusNode2\":" + String(current.statusNode2) +
                  ",\"statusNode3\":" + String(current.statusNode3) + "}";
    request->send(200, "application/json", json);
  });

  server.onNotFound(notFound);
  server.begin();
}
void loop() {
  sendMessage("100," + String(current.statusNode1), MasterNode, Node1);
  sendMessage("200," + String(current.statusNode2), MasterNode, Node2);
  sendMessage("300," + String(current.statusNode3), MasterNode, Node3);
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
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
  if ( sender == 0XBB )
  {
    String status = getValue(incoming, ',', 0);
    incoming = "";
    
    //just for testing purposes
    Serial.print("Node1 received status: ");
    Serial.println(status);

    current.statusNode1 = (status == "true" || status == "1");

    //notify the node you recieved data
    if (status == "true" || status == "1") {
      sendMessage("10", MasterNode, Node1);
      Serial.println("Car present at Node 1");
    } else if (status == "false" || status == "0") {
      sendMessage("10", MasterNode, Node1);
      Serial.println("Car not present at Node 1");
    } else {
      Serial.print("Wrong message from Node1: ");
      Serial.println(status);
    }

    //store the status of the node/slot in permanent flash memory
    preferences.putBool("statusNode1", current.statusNode1);
  }
  if ( sender == 0XCC )
  {
    String status = getValue(incoming, ',', 0);
    incoming = "";
    
    //just for testing purposes
    Serial.print("Node2 received status: ");
    Serial.println(status);

    current.statusNode2 = (status == "true" || status == "1");

    //notify the node you recieved data
    if (status == "true" || status == "1") {
      sendMessage("20", MasterNode, Node2);
      Serial.println("Car present at Node 2");
    } else if (status == "false" || status == "0") {
      sendMessage("20", MasterNode, Node2);
      Serial.println("Car not present at Node 2");
    } else {
      Serial.print("Wrong message from Node2: ");
      Serial.println(status);
    }

    //store the status of the node/slot in permanent flash memory
    preferences.putBool("statusNode2", current.statusNode2);
  }
    if ( sender == 0XDD )
  {
    String status = getValue(incoming, ',', 0);
    incoming = "";
    
    //just for testing purposes
    Serial.print("Node3 received status: ");
    Serial.println(status);

    current.statusNode3 = (status == "true" || status == "1");

    //notify the node you recieved data
    if (status == "true" || status == "1") {
      sendMessage("30", MasterNode, Node3);
      Serial.println("Car present at Node 3");
    } else if (status == "false" || status == "0") {
      sendMessage("30", MasterNode, Node3);
      Serial.println("Car not present at Node 3");
    } else {
      Serial.print("Wrong message from Node3: ");
      Serial.println(status);
    }

    //store the status of the node/slot in permanent flash memory
    preferences.putBool("statusNode3", current.statusNode3);
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

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}