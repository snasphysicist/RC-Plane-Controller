
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
.tbt{\
  grid-template-columns: 1fr 1fr 1fr;\
  grid-template-rows: 1fr 1fr 1fr;\
}\
.u {\
  grid-column: 2;\
  grid-row: 1;\
}\
.d {\
  grid-column: 2;\
  grid-row: 3;\
}\
.l {\
  grid-column: 1;\
  grid-row: 2;\
}\
.r {\
  grid-column: 3;\
  grid-row: 2;\
}\
</style>\
</head>\
<body>\
<h1>Plane Control</h1>\
<div class=\"tbt\">\
  <input id=\"up\" class=\"u\" onmousedown=\"onMouseDown(event)\"\
  onmouseup=\"onMouseUp()\" type=\"button\" value=\"UP\"/>\
  <input id=\"left\" class=\"l\" onmousedown=\"onMouseDown(event)\"\
  onmouseup=\"onMouseUp()\" type=\"button\" value=\"UP\"/>\
  <input id=\"right\" class=\"r\" onmousedown=\"onMouseDown(event)\"\
  onmouseup=\"onMouseUp()\" type=\"button\" value=\"UP\"/>\
  <input id=\"down\" class=\"d\" onmousedown=\"onMouseDown(event)\"\ 
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
  <p>Turn Differential (&#37;)</p>\
  <input id=\"turn-differential\" type=\"number\" min=\"1\" max=\"100\" step=\"1\" value=\"5\"/>\
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
      connection.send(\"U\");\ 
    } else if (whom == \"down\") {\ 
      connection.send(\"D\")\ 
    } else if (whom == \"left\") {\
      connection.send(\"L\");\
    } else if (whom == \"right\") {\
      connection.send(\"R\");\
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
    sendText = sendText + (\"000\" + document.getElementById(\"turn-differential\").value).slice(-3);\
    connection.send(sendText);\
  }\
  let upButton = document.getElementById(\"up\");\
  upButton.addEventListener(\"touchstart\", function(event){onMouseDown(event);});\
  upButton.addEventListener(\"touchend\", onMouseUp);\
  let downButton = document.getElementById(\"down\");\
  downButton.addEventListener(\"touchstart\", function(event){onMouseDown(event);});\
  downButton.addEventListener(\"touchend\", onMouseUp);\
  let leftButton = document.getElementById(\"left\");\
  leftButton.addEventListener(\"touchstart\", function(event){onMouseDown(event);});\
  leftButton.addEventListener(\"touchend\", onMouseUp);\
  let rightButton = document.getElementById(\"right\");\
  rightButton.addEventListener(\"touchstart\", function(event){onMouseDown(event);});\
  rightButton.addEventListener(\"touchend\", onMouseUp);\
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

// PWM duty cycles (initial)
unsigned short int dutyCycleHigh = 1020;
unsigned short int dutyCycleMedium = 610;
unsigned short int dutyCycleLow = 200;

// Add/subtract on turn
unsigned short int dutyCycleTurnDifferential = 51;

// Left/right pins
#define LEFT_MOTOR_GATE   16
#define RIGHT_MOTOR_GATE  4

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
    byte differentialPercentage;
    unsigned short int turnDifferential;
    switch (payload[0]) {
      case 'U':
        // Increase speed
        setLEDDutyCycle(dutyCycleHigh);
        break;
      case 'M':
        // Return to default speed
        setLEDDutyCycle(dutyCycleMedium);
        break;
      case 'D':
        // Low speed
        setLEDDutyCycle(dutyCycleLow);
        break;
      case 'L':
        setTurnDutyCycle('L');
        break;
      case 'R':
        setTurnDutyCycle('R');
        break;
      case 'G':
        // Get power levels
        break;
      case 'S':
        /*
         * The percentages come
         * through as ascii strings
         * Ascii 0 is byte value 48
         * Ascii 1 is byte value 49
         * etc...
         * So subtract 48 to get the digit value
         * Multiply by a power of ten
         * depending upon where they
         * occur in the number
         */
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
        differentialPercentage = (
          100 * (payload[10] - 48)
          + 10 * (payload[11] - 48)
          + (payload[12] - 48)
        );
        dutyCycleHigh = percentageToPWMDutyCycle(highPercentage);
        dutyCycleMedium = percentageToPWMDutyCycle(mediumPercentage);
        dutyCycleLow = percentageToPWMDutyCycle(lowPercentage);
        turnDifferential = percentageToPWMDutyCycle(differentialPercentage);
        dutyCycleTurnDifferential = limitTurnDifferential(turnDifferential);
        setLEDDutyCycle(dutyCycleMedium);
        break;
      default:
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
    LEFT_MOTOR_GATE,
    dutyCycle
  );
  analogWrite(
    RIGHT_MOTOR_GATE,
    dutyCycle
  );
}

/* 
 * Ensure that the turn differential 
 * won't modify the turn cycles
 * outside of 0 to 1023 range
 */
unsigned short int limitTurnDifferential(
  unsigned short int turnDifferential
) {
  unsigned short int minimum = 1023;
  unsigned short int maximum = 0;
  if (minimum > dutyCycleLow) {
    minimum = dutyCycleLow;
  }
  if (minimum > dutyCycleMedium) {
    minimum = dutyCycleMedium;
  }
  if (minimum > dutyCycleHigh) {
    minimum = dutyCycleHigh;
  }
  if (maximum < dutyCycleLow) {
    maximum = dutyCycleLow;
  }
  if (maximum < dutyCycleMedium) {
    maximum = dutyCycleMedium;
  }
  if (maximum < dutyCycleHigh) {
    maximum = dutyCycleHigh;
  }
  if ((1023 - maximum) < minimum) {
    minimum = 1023 - maximum;
  }
  if (turnDifferential < minimum) {
    return turnDifferential;
  } else {
    return minimum;
  }
}

void setTurnDutyCycle(byte turnDirection) {
  if (turnDirection == 'L') {
    /*
     * For left turn
     * Increase right speed
     * Decrease left speed
     */
    analogWrite(
      LEFT_MOTOR_GATE,
      dutyCycleMedium - dutyCycleTurnDifferential
    );
    analogWrite(
      RIGHT_MOTOR_GATE,
      dutyCycleMedium + dutyCycleTurnDifferential
    );
  } else if (turnDirection == 'R') {
    /*
     * For right turn
     * Increase left speed
     * Decrease right speed
     */
    analogWrite(
      LEFT_MOTOR_GATE,
      dutyCycleMedium + dutyCycleTurnDifferential
    );
    analogWrite(
      RIGHT_MOTOR_GATE,
      dutyCycleMedium - dutyCycleTurnDifferential
    );
  }
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
