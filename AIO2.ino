#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char *ssid = "Vivo 1901";
const char *password = "Jul_2822";

// GPIO pins for devices
const int ledPin = D4;
const int fanPin = D5;
const int pumpPin = D6;  // Pump connected to D6

// Variables for sensor data
int sensorValue = 0;
int percent;
bool isManualMode = false;  // Flag to indicate manual/automatic mode
bool isPumpOn = false;      // Flag to track the pump status

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

void setup() {
  Serial.begin(115200); // Initialize serial communication
  pinMode(ledPin, OUTPUT);
  pinMode(fanPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  digitalWrite(ledPin, HIGH);  // Initially turn off the LED
  digitalWrite(fanPin, HIGH);  // Initially turn off the Fan
  digitalWrite(pumpPin, HIGH); // Initially turn off the Pump

  // Connect to Wi-Fi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Route for the root page (displays sensor data and allows control)
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<html><head><script src='https://ajax.googleapis.com/ajax/libs/jquery/3.5.1/jquery.min.js'></script></head><body><h1>ESP8266 Sensor Data & Device Control</h1><div>Mode: <button onclick=\"toggleMode()\">Toggle Manual/Automatic Mode</button></div><div>Current Mode: <span id='mode'>Loading...</span></div><div id='sensor-data'>Loading...</div><div id='manual-controls' style='display:none;'><button onclick=\"controlDevice('led', 'on')\">Turn On LED</button><button onclick=\"controlDevice('led', 'off')\">Turn Off LED</button><button onclick=\"controlDevice('fan', 'on')\">Turn On Fan</button><button onclick=\"controlDevice('fan', 'off')\">Turn Off Fan</button><button onclick=\"controlPump('on')\">Turn On Pump</button><button onclick=\"controlPump('off')\">Turn Off Pump</button></div><script>function updateSensorData() { $.get('/data', function(data) { $('#sensor-data').text('Sensor Data: ' + data + '%'); }); } function updateMode() { $.get('/mode', function(data) { $('#mode').text(data); if(data === 'Manual') { $('#manual-controls').show(); } else { $('#manual-controls').hide(); } }); } function toggleMode() { $.post('/toggleMode'); updateMode(); } function controlDevice(device, status) { $.post('/controlDevice', { device: device, status: status }); } function controlPump(status) { $.post('/controlPump', { status: status }); } setInterval(updateSensorData, 1000); setInterval(updateMode, 1000); </script></body></html>";
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", html);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  // Route to get sensor data
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", String(percent));
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  // Route to get current mode
  server.on("/mode", HTTP_GET, [](AsyncWebServerRequest *request) {
    String mode = isManualMode ? "Manual" : "Automatic";
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", mode);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  // Route to toggle mode
  server.on("/toggleMode", HTTP_POST, [](AsyncWebServerRequest *request) {
    isManualMode = !isManualMode;
    if (!isManualMode) {
      digitalWrite(pumpPin, HIGH); // Turn off pump in automatic mode initially
    }
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Mode toggled");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  // Route to control pump manually
  server.on("/controlPump", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("status", true)) {
      String status = request->getParam("status", true)->value();
      if (isManualMode) {
        if (status == "on") {
          digitalWrite(pumpPin, LOW); // Turn on pump
          isPumpOn = true;
        } else if (status == "off") {
          digitalWrite(pumpPin, HIGH); // Turn off pump
          isPumpOn = false;
        }
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Pump status updated");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
      } else {
        AsyncWebServerResponse *response = request->beginResponse(400, "text/plain", "Pump control only available in manual mode");
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
      }
    } else {
      AsyncWebServerResponse *response = request->beginResponse(400, "text/plain", "Missing status parameter");
      response->addHeader("Access-Control-Allow-Origin", "*");
      request->send(response);
    }
  });

  // Route to control devices (LED and Fan)
  server.on("/controlDevice", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("device", true) && request->hasParam("status", true)) {
      String device = request->getParam("device", true)->value();
      String status = request->getParam("status", true)->value();
      Serial.print("Received request: Device = ");
      Serial.print(device);
      Serial.print(", Status = ");
      Serial.println(status);

      AsyncWebServerResponse* response;

      if (device == "led") {
        if (status == "on") {
          digitalWrite(ledPin, LOW); // Turn on LED (assuming LOW is ON)
          response = request->beginResponse(200, "text/plain", "LED turned on");
        } else if (status == "off") {
          digitalWrite(ledPin, HIGH); // Turn off LED (assuming HIGH is OFF)
          response = request->beginResponse(200, "text/plain", "LED turned off");
        } else {
          response = request->beginResponse(400, "text/plain", "Invalid status for LED");
        }
      } else if (device == "fan") {
        if (status == "on") {
          digitalWrite(fanPin, LOW); // Turn on Fan (assuming LOW is ON)
          response = request->beginResponse(200, "text/plain", "Fan turned on");
        } else if (status == "off") {
          digitalWrite(fanPin, HIGH); // Turn off Fan (assuming HIGH is OFF)
          response = request->beginResponse(200, "text/plain", "Fan turned off");
        } else {
          response = request->beginResponse(400, "text/plain", "Invalid status for Fan");
        }
      } else {
        response = request->beginResponse(400, "text/plain", "Invalid device");
      }

      response->addHeader("Access-Control-Allow-Origin", "*");
      response->addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
      response->addHeader("Access-Control-Allow-Headers", "Content-Type");
      request->send(response);
    } else {
      Serial.println("Error: Missing device or status parameter");
      AsyncWebServerResponse* response = request->beginResponse(400, "text/plain", "Missing device or status parameter");
      response->addHeader("Access-Control-Allow-Origin", "*");
      response->addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
      response->addHeader("Access-Control-Allow-Headers", "Content-Type");
      request->send(response);
    }
  });

  // Route to handle CORS preflight requests
  server.on("/controlDevice", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse* response = request->beginResponse(204); // No content
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
    request->send(response);
  });

  // Route to handle the '/update' request (assuming you need this route)
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Handle the update request as needed
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Update received");
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
  });

  // Start server
  server.begin();
}

void loop() {
  updateSensorValue(); // Update sensor value

  if (!isManualMode) {
    // Automatic pump control logic
    if (percent < 30 && !isPumpOn) {  // Turn on pump if moisture level is below 30%
      digitalWrite(pumpPin, LOW);
      isPumpOn = true;
      Serial.println("Pump turned on automatically");
    } else if (percent >= 70 && isPumpOn) {  // Turn off pump if moisture level is above 70%
      digitalWrite(pumpPin, HIGH);
      isPumpOn = false;
      Serial.println("Pump turned off automatically");
    }
  }

  delay(2000); // Delay before updating again
}

// Function to update sensor value
void updateSensorValue() {
  sensorValue = random(0,1024); // Read moisture sensor value
  percent = map(sensorValue, 1024, 0, 0, 100); // Convert to percentage
  Serial.print("Moisture Level: ");
  Serial.print(percent);
  Serial.println("%");
}
