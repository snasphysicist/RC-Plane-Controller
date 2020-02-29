
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
<h1>Plane Control</h1>\
<div>\
  <input id=\"up\" onmousedown=\"onMouseDown(event)\"\
  onmouseup=\"onMouseUp()\" type=\"button\" value=\"UP\"/>\
  <input id=\"down\" onmousedown=\"onMouseDown(event)\"\ 
  onmouseup=\"onMouseUp()\" type=\"button\" value=\"DOWN\"/>\ 
</div>\
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
  let upButton = document.getElementById(\"up\");\
  upButton.addEventListener(\"touchstart\", function(event){onMouseDown(event);});\
  upButton.addEventListener(\"touchend\", onMouseUp);\
  let downButton = document.getElementById(\"down\");\
  downButton.addEventListener(\"touchstart\", function(event){onMouseDown(event);});\
  downButton.addEventListener(\"touchend\", onMouseUp);\
</script>\ 
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
#define DUTY_CYCLE_HIGH             1020
#define DUTY_CYCLE_MEDIUM           620
#define DUTY_CYCLE_LOW              220


// Web server handler function
void handleRequest() {
  webServer.send(
    STATUS_CODE,
    CONTENT_TYPE,
    PAGE_CONTENT
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
    switch (payload[0]) {
      case 'H':
        // Increase speed
        setLEDDutyCycle(DUTY_CYCLE_HIGH);
        break;
      case 'L':
        // Low speed
        setLEDDutyCycle(DUTY_CYCLE_LOW);
        break;
      default:
        // Return to default speed
        setLEDDutyCycle(DUTY_CYCLE_MEDIUM);
        break;
    }
  }
}

void setLEDDutyCycle(int dutyCycle) {
  analogWrite(
    LED_BUILTIN,
    dutyCycle
  );
}

void setup() {

  // Assign request handler to root route
  webServer.on(
    "/",
    handleRequest
  );

  // Start web server
  webServer.begin();

  // Assign message handler to web socket server
  webSocketServer.onEvent(handleWebSocketInput);

  // Start web socket server
  webSocketServer.begin();

  // Wifi in access point mode
  WiFi.mode(WIFI_AP);

  // Set access point ip address
  WiFi.softAPConfig(
    webServerIpAddress,
    webServerIpAddress,
    IPAddress(255, 255, 255, 0)
  );

  // Start up access point
  WiFi.softAP(
    ACCESS_POINT_SSID,
    ACCESS_POINT_PSK
  );

  // Start up DNS server
  dnsServer.start(
    DNS_SERVER_PORT,
    "*",
    webServerIpAddress
  );

  // Turn on LED
  setLEDDutyCycle(DUTY_CYCLE_MEDIUM);
}

void loop() {
  // Handle DNS requests
  dnsServer.processNextRequest();
  // Handle requests to HTTP server
  webServer.handleClient();
  // Handle data to web socket server
  webSocketServer.loop();
}
