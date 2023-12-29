#include <SPI.h>
#include <RadioLib.h>
#include <Adafruit_GFX.h>
#include <Preferences.h>
#include <WiFi.h>
#include <ESPAsyncWebSrv.h>

#define RADIO_NSS_PIN 5        // Pin connected to the NSS (Chip Select) of the LoRa module
#define RADIO_RST_PIN 14       // Pin connected to the reset of the LoRa module
#define RADIO_DIO0_PIN 2       // Pin connected to DIO0 of the LoRa module
#define RADIO_DIO1_PIN 4       // Pin connected to DIO1 of the LoRa module

// Module* module;               // Pointer to the RadioLib module
// SX1278* radio;                 // Pointer to the SX1278 radio module

byte MasterNode = 0xFF;       // Master node address
byte Node1 = 0xBB;            // Node 1 address
byte Node2 = 0xCC;            // Node 2 address
byte Node3 = 0xDD;            // Node 3 address

String SenderNode = "";       // Variable to store the sender node ID
String outgoing;              // Outgoing message
byte msgCount = 0;            // Count of outgoing messages
String incoming = "";         // Incoming message

unsigned long previousMillis = 0;        // Time since the last event fired
unsigned long int previoussecs = 0;      // Previous seconds
unsigned long int currentsecs = 0;       // Current seconds
unsigned long currentMillis = 0;         // Current milliseconds
int interval = 1;             // Interval for updating every 1 second

struct Status {
  bool statusNode1;           // Status of Node 1
  bool statusNode2;           // Status of Node 2
  bool statusNode3;           // Status of Node 3
};

Status current;                // Current status of nodes

Preferences preferences;       // Preferences object for storing data in flash memory

const char *ssid = "BlazingSpeed-TIME_AB4F";  // WiFi SSID
const char *password = "ng96samM";             // WiFi password

AsyncWebServer server(80);     // AsyncWebServer instance for handling web requests

unsigned long occupiedStartTimeNode2 = 0;    // Store the time when Node 2 becomes occupied

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

// Initialize LoRa module
// module = new Module(RADIO_NSS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN, RADIO_DIO1_PIN);
// radio = new SX1278(module);
// SX1278 radio = new Module(5, 2, 14, 2);
// SX1278 radio = new Module(RADIO_NSS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN, RADIO_DIO1_PIN);
Module module(RADIO_NSS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN, RADIO_DIO1_PIN);
SX1278 radio(&module);

void setup() {
  Serial.begin(115200);
  delay(500);

  while (!Serial);
  Serial.println("LoRa Master Node");

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
    while(1);
  }
  // int state = radio.begin();
  // if (state == RADIOLIB_ERR_NONE) {
  //   Serial.println(F("success!"));
  // } else {
  //   Serial.print(F("failed, code "));
  //   Serial.println(state);
  //   while (true);
  // }

  Serial.println("Radio initialized successfully");

  // Open preferences for the application
  preferences.begin("my-app", false);
  current.statusNode1 = preferences.getBool("statusNode1", false);
  current.statusNode2 = preferences.getBool("statusNode2", false);
  current.statusNode3 = preferences.getBool("statusNode3", false);

  //Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.print("Connected to WiFi with IP: ");
  Serial.println(WiFi.localIP());

  // Set up routes for web server
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
  // Send status messages to nodes
  sendMessage("100," + String(current.statusNode1), MasterNode, Node1);
  sendMessage("200," + String(current.statusNode2), MasterNode, Node2);
  sendMessage("300," + String(current.statusNode3), MasterNode, Node3);

  Serial.println("Successfully sent ready signals to all slave nodes");

  // Check if there is an incoming message
  if (radio.available()) {
    int16_t rssi = radio.getRSSI();  // Get RSSI value
    size_t packetSize = radio.getPacketLength();  // Get the length of the received data
    uint8_t buffer[packetSize];

    // Read the incoming message into the buffer
    radio.readData(buffer, packetSize);

    // Call onReceive with the necessary parameters
    onReceive(rssi, buffer, packetSize);
  }

  delay(100);  // Add a small delay to prevent excessive checking
}

// Function to send a message to a node
void sendMessage(String outgoing, byte MasterNode, byte otherNode) {
  // Convert the String to a char array
  char msg[outgoing.length() + 1];
  outgoing.toCharArray(msg, outgoing.length() + 1);

  // Transmit the message
  radio.transmit((uint8_t*)msg, outgoing.length() + 1, otherNode);
  msgCount++;  // Increment message ID
}


// Function to handle the reception of messages
void onReceive(int16_t rssi, uint8_t* buffer, size_t size) {
  // Convert the received buffer to a String
  String incoming((char*)buffer);
  
  // Check if there is data available in the radio buffer
  if (!radio.available()) return;

  // Read packet header bytes:
  int recipient = radio.read();          // recipient address
  byte sender = radio.read();            // sender address

  // Determine the sender node based on the sender address
  if (sender == 0xBB)
    SenderNode = "Node1:";
  if (sender == 0xCC)
    SenderNode = "Node2:";
  if (sender == 0xDD)
    SenderNode = "Node3:";

  // Read additional header information
  byte incomingMsgId = radio.read();     // incoming msg ID
  byte incomingLength = radio.read();    // incoming msg length

  // Read the rest of the message
  while (radio.available()) {
    incoming += (char)radio.read();
  }

  // Check if the received message length matches the expected length
  if (incomingLength != incoming.length()) {
    // Print an error message and skip the rest of the function
    //Serial.println("error: message length does not match length");
    return;
  }

  // Print details of the received message
  // Serial.println("Received from: 0x" + String(sender, HEX));
  // Serial.println("Sent to: 0x" + String(recipient, HEX));
  // Serial.println("Message ID: " + String(incomingMsgId));
  // Serial.println("Message length: " + String(incomingLength));
  // Serial.println("Message: " + incoming);
  // Serial.println("RSSI: " + String(rssi));
  // Serial.println();

  // Process messages based on the sender node
  if (sender == 0xBB) {
    // Process message from Node 1
    processNodeStatus(incoming, current.statusNode1, "Node1", "10");
  } else if (sender == 0xCC) {
    // Process message from Node 2
    processNodeStatus(incoming, current.statusNode2, "Node2", "20");
  } else if (sender == 0xDD) {
    // Process message from Node 3
    processNodeStatus(incoming, current.statusNode3, "Node3", "30");
  }
}

// Function to extract a value from a string based on a separator and index
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

// Function to handle a request for an unknown route
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

// Function to process the status message from a node
void processNodeStatus(String statusMessage, bool& currentNodeStatus, const char* nodeName, const char* notificationCode) {
  // Extract the status value from the statusMessage
  String status = getValue(statusMessage, ',', 0);
  
  // Clear the incoming buffer
  statusMessage = "";

  // Print the received status for testing purposes
  Serial.print(nodeName);
  Serial.print(" received status: ");
  Serial.println(status);

  // Update the current node status based on the received status
  currentNodeStatus = (status == "true" || status == "1");

  // Notify the master node about the received data
  sendMessage(notificationCode, MasterNode, nodeName[4] - '0');

  // Print a message based on the received status
  if (currentNodeStatus) {
    Serial.print("Car present at ");
    Serial.println(nodeName);
  } else {
    Serial.print("Car not present at ");
    Serial.println(nodeName);
  }

  // Print a message for incorrect messages
  if (!(status == "true" || status == "1" || status == "false" || status == "0")) {
    Serial.print("Wrong message from ");
    Serial.print(nodeName);
    Serial.print(": ");
    Serial.println(status);
  }

  // Store the status of the node/slot in permanent flash memory
  const char* key = ("status" + String(nodeName[4] - '0')).c_str();
  preferences.putBool(key, currentNodeStatus);
}        
