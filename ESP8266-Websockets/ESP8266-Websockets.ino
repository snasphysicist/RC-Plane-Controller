
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <DNSServer.h>

// Access point constants
#define ACCESS_POINT_SSID   "RCPLANE0"
#define ACCESS_POINT_PSK    "IbelieveIcanFLY"

// Server IP address
#define OCTET_0             192
#define OCTET_1             168
#define OCTET_2             4
#define OCTET_3             1
IPAddress webServerIpAddress(
  OCTET_0,
  OCTET_1,
  OCTET_2,
  OCTET_3
);

// Web server constants
#define WEB_SERVER_PORT     80
#define STATUS_CODE         200
#define CONTENT_TYPE        "text/html"

// Content of page served
#define PAGE_CONTENT\ 
"\
<!DOCTYPE html>\
<html>\
<head>\
<style>\
body {\
  display: grid;\
  justify-items: center;\
  width: 100%;\
  font-family: sans;\
  background-color: black;\
  color: white;\
}\
div {\
  display: grid;\
  justify-items: center;\
  width: 100%;\
}\
input {\
  display: block;\
  min-width: 50%;\
  height: 100px;\
  font-size: 2em;\
  font-weight: 700;\
  margin: 15px;\
  background-color: black;\
  color: white;\
  border-color: white;\
  border-style: solid;\
}\
input:active{\
  background-color: #C92020;\
}\
</style>\
</head>\
<body>\
<h1>Plane Control</h1>\
<div>\
  <input id=\"up\" onmousedown=\"onMouseDown(event)\"\
  onmouseup=\"onMouseUp()\" type=\"button\" value=\"UP\"/>\
  <input id=\"down\" onmousedown=\"onMouseDown(event)\"\ 
  onmouseup=\"onMouseUp()\" type=\"button\" value=\"DOWN\"/>\ 
</div>\
<div>\
  <h2>Power Settings</h2>\
  <p>High Power (&#37;)</p>\
  <input id=\"high-power\" type=\"number\" min=\"1\" max=\"100\" step=\"1\" value=\"100\"/>\
  <p>Medium Power (&#37;)</p>\
  <input id=\"medium-power\" type=\"number\" min=\"1\" max=\"100\" step=\"1\" value=\"60\"/>\
  <p>Low Power (&#37;)</p>\
  <input id=\"low-power\" type=\"number\" min=\"1\" max=\"100\" step=\"1\" value=\"20\"/>\
  <input id=\"set\" onclick=\"setDutyCycles()\" type=\"button\" value=\"SET\"/>\  
<div>\
<script type=\"text/javascript\">\ 
  let connection = new WebSocket(\
    \"ws://\" + location.hostname + \":9012\",\
    [\"arduino\"]\
  );\
  function onMouseDown(event) {\ 
    let whom = event.target.id;\ 
    if (whom == \"up\") {\ 
      connection.send(\"H\");\ 
    } else {\ 
      connection.send(\"L\")\ 
    }\ 
  }\ 
  function onMouseUp() {\ 
    connection.send(\"M\")\ 
  }\
  function setDutyCycles() {\ 
    let sendText = \"S\";\
    sendText = sendText + (\"000\" + document.getElementById(\"high-power\").value).slice(-3);\
    sendText = sendText + (\"000\" + document.getElementById(\"medium-power\").value).slice(-3);\
    sendText = sendText + (\"000\" + document.getElementById(\"low-power\").value).slice(-3);\
    connection.send(sendText);\
  }\
  let upButton = document.getElementById(\"up\");\
  upButton.addEventListener(\"touchstart\", function(event){onMouseDown(event);});\
  upButton.addEventListener(\"touchend\", onMouseUp);\
  let downButton = document.getElementById(\"down\");\
  downButton.addEventListener(\"touchstart\", function(event){onMouseDown(event);});\
  downButton.addEventListener(\"touchend\", onMouseUp);\
</script>\
</body>\
</html>\
"

// Web server object
ESP8266WebServer webServer(WEB_SERVER_PORT);

// Websocket server constants
#define WEB_SOCKET_SERVER_PORT      9012

// Websocket server object
WebSocketsServer webSocketServer(WEB_SOCKET_SERVER_PORT);

// DNS Port 
#define DNS_SERVER_PORT             53

// DNS server object
DNSServer dnsServer;

// PWM duty cycles
unsigned short int dutyCycleHigh = 1020;
unsigned short int dutyCycleMedium = 610;
unsigned short int dutyCycleLow = 200;

// Web server handler function
void handleRequest() {
  webServer.send(
    STATUS_CODE,
    CONTENT_TYPE,
    F(PAGE_CONTENT)
  );
}

// Websocket server handler function
void handleWebSocketInput(
  uint8_t number,
  WStype_t messageType,
  uint8_t *payload,
  size_t length
) {
  if (messageType == WStype_TEXT) {
    byte highPercentage;
    byte mediumPercentage;
    byte lowPercentage;
    switch (payload[0]) {
      case 'H':
        // Increase speed
        setLEDDutyCycle(dutyCycleHigh);
        break;
      case 'L':
        // Low speed
        setLEDDutyCycle(dutyCycleLow);
        break;
      case 'G':
        // Get power levels
        break;
      case 'S':
        highPercentage = (
          100 * (payload[1] - 48)
          + 10 * (payload[2] - 48)
          + (payload[3] - 48)
        );
        mediumPercentage = (
          100 * (payload[4] - 48)
          + 10 * (payload[5] - 48)
          + (payload[6] - 48)
        );
        lowPercentage = (
          100 * (payload[7] - 48)
          + 10 * (payload[8] - 48)
          + (payload[9] - 48)
        );
        dutyCycleHigh = percentageToPWMDutyCycle(highPercentage);
        dutyCycleMedium = percentageToPWMDutyCycle(mediumPercentage);
        dutyCycleLow = percentageToPWMDutyCycle(lowPercentage);
        setLEDDutyCycle(dutyCycleMedium);
        break;
      default:
        // Return to default speed
        setLEDDutyCycle(dutyCycleMedium);
        break;
    }
  }
}

unsigned short int percentageToPWMDutyCycle(byte percentage) {
  return (unsigned short int) ((((float) percentage) / 100.0) * 1023);
}

byte pwmDutyCycleToPercentage(unsigned short int dutyCycle) {

}

void setLEDDutyCycle(int dutyCycle) {
  analogWrite(
    LED_BUILTIN,
    dutyCycle
  );
}

void setup() {

  Serial.begin(38400);

  Serial.println("Open Serial");

  // Assign request handler to root route
  webServer.on(
    "/",
    handleRequest
  );

  Serial.println("Set Web Server");

  // Start web server
  webServer.begin();

  Serial.println("Start Web Server");

  // Assign message handler to web socket server
  webSocketServer.onEvent(handleWebSocketInput);

  Serial.println("Set Web Socket Server");

  // Start web socket server
  webSocketServer.begin();

  Serial.println("Start Web Socket Server");

  // Wifi in access point mode
  WiFi.mode(WIFI_AP);

  Serial.println("Set Wifi Mode");


  // Set access point ip address
  WiFi.softAPConfig(
    webServerIpAddress,
    webServerIpAddress,
    IPAddress(255, 255, 255, 0)
  );

  Serial.println("Set Wifi IP Address");

  // Start up access point
  WiFi.softAP(
    ACCESS_POINT_SSID,
    ACCESS_POINT_PSK
  );

  Serial.println("Start Access Point");

  // Start up DNS server
  dnsServer.start(
    DNS_SERVER_PORT,
    "*",
    webServerIpAddress
  );

  Serial.println("Start DNS Server");

  // Turn on LED
  setLEDDutyCycle(dutyCycleMedium);

  Serial.println("Turn LED On");

}

int i = 0;

void loop() {
  // Serial.print(i);
  // Serial.println(" LOOP");
  // Handle DNS requests
  // Serial.println("Handle DNS");
  dnsServer.processNextRequest();
  // Handle requests to HTTP server
  // Serial.println("Handle Web Server");
  webServer.handleClient();
  // Handle data to web socket server
  // Serial.println("Handle Web Socket Server");
  webSocketServer.loop();
  i++;
}
