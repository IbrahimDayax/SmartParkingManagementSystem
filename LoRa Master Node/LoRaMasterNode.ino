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
// const char *password = "ekkc2024";
const char *ssid = "BlazingSpeed-TIME_AB4F";
const char *password = "ng96samM";

AsyncWebServer server(80);

unsigned long occupiedTime1 = 0; //Store the time that Parking Slot 1 is occupied
unsigned long occupiedTime2; //Store the time that Parking Slot 2 is occupied
unsigned long occupiedTime3; //Store the time that Parking Slot 3 is occupied

unsigned long occupiedStartTime1;
unsigned long occupiedEndTime1;
unsigned long occupiedStartTime2;
unsigned long occupiedEndTime2;
unsigned long occupiedStartTime3;
unsigned long occupiedEndTime3;

String elapsedTime1 = ""; //Store the time that Parking Slot 1 is occupied
String elapsedTime2 = ""; //Store the time that Parking Slot 2 is occupied
String elapsedTime3 = ""; //Store the time that Parking Slot 3 is occupied

// HTML, CSS, and JS code stored in program memory
const char index_html[] PROGMEM =R"rawliteral(
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body {
            font-family: 'Poppins', sans-serif;
            margin: 0;
            padding: 20px;
            text-align: center;
            background-color: #42a1f5;
        }

        h1 {
            color: #333;
        }

        .parking-grid {
            display: flex;
            flex-wrap: wrap;
            justify-content: center;
            gap: 10px;
        }

        .parking-space {
            width: calc((100% - 60px) / 5); /* Adjusted width to fit 5 per row */
            height: 300px; /* Adjusted height */
            background-color: green;
            border: 2px dashed #fff; /* Border on all sides */
            border-bottom: none; /* No border at the bottom */
            position: relative;
        }

        .occupied {
            background-color: red;
        }

        .available {
            background-color: green;
        }

        .parking-text {
            font-size: 12px;
            color: #fff;
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
        }

        .legend {
            position: absolute;
            top: 10px;
            right: 10px;
            font-size: 14px;
        }

        .legend-item {
            margin-bottom: 5px;
        }

        .legend-square {
            display: inline-block;
            width: 20px;
            height: 20px;
            margin-right: 5px;
            border-radius: 3px;
        }

        .datetime {
            position: absolute;
            top: 10px;
            left: 10px;
            font-size: 18px;
            color: #fff;
        }

        .weather {
            position: absolute;
            bottom: 10px;
            left: 50%;
            transform: translateX(-50%);
            font-size: 16px;
            color: #fff;
        }
    </style>
    <title>Parking Grid</title>
</head>

<body>
    <div class="datetime" id="datetime"></div>
    <h1>Parking Grid</h1>
    <div class="legend">
      <div class="legend-item">
          <div class="legend-square occupied"></div>
          Occupied
      </div>
      <div class="legend-item">
          <div class="legend-square available"></div>
          Available
      </div>
  </div>
    <div class="parking-grid">
        <!-- Parking spaces -->
        <!-- Row 1 -->
        <div class="parking-space" id="Node1">
            <div class="parking-text">1</div>
        </div>
        <div class="parking-space" id="Node2">
            <div class="parking-text">2</div>
        </div>
        <div class="parking-space" id="Node3">
            <div class="parking-text">3</div>
        </div>
        <div class="parking-space" id="Node4">
            <div class="parking-text">4</div>
        </div>
        <div class="parking-space" id="Node5">
            <div class="parking-text">5</div>
        </div>
        <!-- Add more parking spaces as needed -->
        <!-- Row 2 -->
        <div class="parking-space" id="Node6">
            <div class="parking-text">6</div>
        </div>
        <div class="parking-space" id="Node7">
            <div class="parking-text">7</div>
        </div>
        <div class="parking-space" id="Node8">
            <div class="parking-text">8</div>
        </div>
        <div class="parking-space" id="Node9">
            <div class="parking-text">9</div>
        </div>
        <div class="parking-space" id="Node10">
            <div class="parking-text">10</div>
        </div>
        <!-- Add more parking spaces as needed -->
    </div>

    <script>
        function updateNodeStatus(nodeId, isOccupied, elapsedTime) {
            var nodeElement = document.getElementById(nodeId);
            if (isOccupied) {
                nodeElement.classList.add('occupied');
                nodeElement.querySelector('.parking-text').textContent = elapsedTime;
            } else {
                nodeElement.classList.remove('occupied');
                nodeElement.querySelector('.parking-text').textContent = extractNumFromStr(nodeId);
            }
        }

        function updateParkingStatus(statusNode1, statusNode2, statusNode3, elapsedTime1, elapsedTime2, elapsedTime3) {
            updateNodeStatus('Node1', statusNode1, elapsedTime1);
            updateNodeStatus('Node2', statusNode2, elapsedTime2);
            updateNodeStatus('Node3', statusNode3, elapsedTime3);
            // Add more nodes as needed
        }

        function extractNumFromStr(str) {
            // Use regular expression to match the digits at the end of the string
            const match = str.match(/\d+$/);
            
            // If there's a match, return the matched digits, otherwise return null
            return match ? match[0] : null;
        }

        function fetchAndUpdate() {
            fetch('/status') // Assuming you have a new route for fetching status
                .then(response => response.json())
                .then(data => updateParkingStatus(data.statusNode1, data.statusNode2, data.statusNode3, data.elapsedTime1, data.elapsedTime2, data.elapsedTime3))
                .catch(error => console.error('Error fetching status and occupiedTime:', error));
        }

        window.onload = function() {
            fetchAndUpdate();
            setInterval(fetchAndUpdate, 1000); // Update every 1 second (adjust as needed)
            // Update datetime
            updateDateTime();
            setInterval(updateDateTime, 1000); // Update every second
        };

        // Function to update date and time
        function updateDateTime() {
            const now = new Date();
            const options = { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric', hour: 'numeric', minute: 'numeric', second: 'numeric' };
            const formattedDateTime = now.toLocaleDateString('en-US', options);
            document.getElementById('datetime').textContent = formattedDateTime;
        }
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
  statusNode2 = preferences.getBool("statusNode2", false);
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
              ",\"statusNode3\":" + String(statusNode3) +
              ",\"elapsedTime1\":\"" + elapsedTime1 + "\"" + 
              ",\"elapsedTime2\":\"" + elapsedTime2 + "\"" +
              ",\"elapsedTime3\":\"" + elapsedTime3 + "\"" + "}";

    request->send(200, "application/json", json);
  });

  server.onNotFound(notFound);
  server.begin();

  refMagTime = millis();
}

void loop() {
  currentMillis = millis();
  currentSecs = currentMillis / 1000;

  if (statusNode1 == true) {
    occupiedTime1 = millis() - occupiedStartTime1;
    //Serial.print("Node 1 occupied Time: ");
    elapsedTime1 = millisToHoursMinutesSeconds(occupiedTime1);
    //Serial.println(elapsedTime1);
  }

  if (statusNode2 == true) {
    occupiedTime2 = millis() - occupiedStartTime2;
    //Serial.print("Node 2 occupied Time: ");
    elapsedTime2 = millisToHoursMinutesSeconds(occupiedTime2);
    //Serial.println(elapsedTime2);
  }

  if (statusNode3 == true) {
    occupiedTime3 = millis() - occupiedStartTime3;
    //Serial.print("Node 3 occupied Time: ");
    elapsedTime3 = millisToHoursMinutesSeconds(occupiedTime3);
    //Serial.println(elapsedTime3);
  }

  if ((unsigned long)(currentSecs - previousSecs) >= interval) {
    seconds = seconds + 1;
    if ( seconds > 5 )
    { 
      seconds = 0;
    }
    if ( (seconds >= 0) && (seconds <= 1) )
    {
      message = "20, " + String(statusNode1) + " ," + String(statusNode2) + " ," + String(statusNode3);
      sendMessage(message, MasterNode, Node1, Node2, Node3);
    }
    if ( (seconds >= 2 ) && (seconds <= 3))
    {
      message = "20, " + String(statusNode1) + " ," + String(statusNode2) + " ," + String(statusNode3);
      sendMessage(message, MasterNode, Node1, Node2, Node3);
    }
    if ( (seconds >= 4 ) && (seconds <= 5))
    {
      message = "20, " + String(statusNode1) + " ," + String(statusNode2) + " ," + String(statusNode3);
      sendMessage(message, MasterNode, Node1, Node2, Node3);
    }
    previousSecs = currentSecs;
  }


  // message = "20, " + String(statusNode1) + " ," + String(statusNode2) + " ," + String(statusNode3);
  // sendMessage(message, MasterNode, Node1, Node2, Node3);
  // delay(1000);
  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
  // delay(3000);
}

void sendMessage(String outgoing, byte MasterNode, byte slaveNode1, byte slaveNode2, byte slaveNode3) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(slaveNode1);              // add destination address
  LoRa.write(slaveNode2);              // add destination address
  LoRa.write(slaveNode3);              // add destination address
  LoRa.write(MasterNode);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}
void onReceive(int packetSize) {
  if (packetSize == 0) {
    //Serial.println("Packet Size is 0");
    return;
  }          // if there's no packet, return
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
    String status = getValue(incoming, ',', 0);
    incoming = "";
    
    //just for testing purposes
    // Serial.print("From Node1 received status: ");
    // Serial.println(status);

    if (status.toInt() == 1) {
      statusNode1 = true;
      occupiedStartTime1 = millis();
    } else if (status.toInt() == 0) {
      statusNode1 = false;
      occupiedEndTime1 = millis();
      occupiedTime1 = 0;
    } 
    

    // Serial.print("statusNode1: ");
    // Serial.println(statusNode1);

    // if (statusNode1 == false) {
    //   occupiedTime1 = occupiedEndTime1 - occupiedStartTime1;
    //   Serial.print("Occupied Time: ");
    //   elapsedTime1 = millisToHoursMinutesSeconds(occupiedTime1);
    //   Serial.println(elapsedTime1);
    // }

    //store the status of the node/slot in permanent flash memory
    preferences.putBool("statusNode1", statusNode1);
  }
  if ( sender == 0XCC )
  {
    String status = getValue(incoming, ',', 0);
    incoming = "";
    
    //just for testing purposes
    // Serial.print("From Node2 received status: ");
    // Serial.println(status);

    if (status.toInt() == 1) {
      statusNode2 = true;
      occupiedStartTime2 = millis();
    } else if (status.toInt() == 0) {
      statusNode2 = false;
      occupiedEndTime2 = millis();
      occupiedTime2 = 0;
    } 

    // Serial.print("statusNode2: ");
    // Serial.println(statusNode2);

    // if (statusNode2 == false) {
    //   occupiedTime2 = occupiedEndTime2 - occupiedStartTime2;
    //   Serial.print("Occupied Time: ");
    //   elapsedTime2 = millisToHoursMinutesSeconds(occupiedTime2);
    //   Serial.println(elapsedTime2);
    // }

    //store the status of the node/slot in permanent flash memory
    preferences.putBool("statusNode2", statusNode2);
  }
    if ( sender == 0XDD )
  {
    String status = getValue(incoming, ',', 0);
    incoming = "";

    unsigned long occupiedStartTime;
    unsigned long occupiedEndTime;
    
    // //just for testing purposes
    // Serial.print("From Node3 received status: ");
    // Serial.println(status);

    if (status.toInt() == 1) {
      statusNode3 = true;
      occupiedStartTime3 = millis();
    } else if (status.toInt() == 0) {
      statusNode3 = false;
      occupiedEndTime3 = millis();
      occupiedTime3 = 0;
    } 

    // Serial.print("statusNode3: ");
    // Serial.println(statusNode3);

    // if (statusNode3 == false) {
    //   occupiedTime3 = occupiedEndTime3 - occupiedStartTime3;
    //   Serial.print("Occupied Time: ");
    //   elapsedTime3 = millisToHoursMinutesSeconds(occupiedTime3);
    //   Serial.println(elapsedTime3);
    // }

    //store the status of the node/slot in permanent flash memory
    preferences.putBool("statusNode3", statusNode3);
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

String millisToHoursMinutesSeconds(long millis) {
  String s, m, h;
  long seconds = millis / 1000;
  long minutes = seconds / 60;
  long hours = minutes / 60;
  seconds = seconds % 60;
  minutes = minutes % 60;

  // Format hours, minutes, and seconds with leading zeros if less than 10
  h = (hours < 10) ? "0" + String(hours) : String(hours);
  m = (minutes < 10) ? "0" + String(minutes) : String(minutes);
  s = (seconds < 10) ? "0" + String(seconds) : String(seconds);

  return h + ":" + m + ":" + s;
}


void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

bool stringToBool(const String& str) {
  return str == "1";
}
