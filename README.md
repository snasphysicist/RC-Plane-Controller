# RC Plane Controller Code

This code is intended to control a very simple remote controlled plane, over WiFi, via an ESP8266.

The ESP8266 provides:

- A WiFi access point which requires a password
- A web server to serve the 'web application' used to control the plane
- A web socket server which receives commands from the web application
- A DNS server so a human readable url can be used to access the application instead of an IP address

Only up/down controls are implemented. This is achieved by varying the motor speed via PWM control.

This code is being developed for use in an 'Introduction to Making' workshop in the A*STAR Makespace in Singapore, hence its simplicitly
