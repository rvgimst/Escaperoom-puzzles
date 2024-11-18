/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-web-server-slider-pwm/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

/*
 * RVG: augmented with different functionality:
 * added FastLED to control WS2811 LED strip
 * slider to control brightness (0-255)
 * slider to control hue/color
 */

// Import required WiFi libraries
#include <WiFiConfig.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Replace with your network credentials
//const char* ssid = "REPLACE_WITH_YOUR_SSID";
//const char* password = "REPLACE_WITH_YOUR_PASSWORD";

// FastLED includes
#include <FastLED.h>

// FastLED defines
#define NUM_LEDS 12 // short Strip 
#define DATA_PIN 12 // ESP32-CAM: GPIO12/HS2_DATA3
#define LED_TYPE WS2811
#define COLOR_ORDER GRB

// FastLED parameters
CRGB leds[NUM_LEDS];
int LEDBrightness; 
int LEDHue; 

// Web page input parameters
String slider1Value = "60";
String slider2Value = "20";
const char* PARAM_INPUT = "value";

// PWM props /LED for testing
const int output = 2;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP Web Server</title>
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 2.3rem;}
    p {font-size: 1.9rem;}
    body {max-width: 400px; margin:0px auto; padding-bottom: 25px;}
    .slider { -webkit-appearance: none; margin: 14px; width: 360px; height: 25px; background: #FFD65C;
      outline: none; -webkit-transition: .2s; transition: opacity .2s;}
    .slider::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 35px; height: 35px; background: #003249; cursor: pointer;}
    .slider::-moz-range-thumb { width: 35px; height: 35px; background: #003249; cursor: pointer; } 
    .button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px; text-decoration: none; font-size: 20px; margin: 2px; cursor: pointer; }
    .button2 { background-color: #555555; }
  </style>
</head>
<body>
  <h2>Ambient LED Control</h2>
  <p><i>[ESP32 Web Server]</i></p>
  <p><span id="textSlider1Value">Brightness: %SLIDER1VALUE%</span></p>
  <p><input type="range" onchange="updateSlider1PWM(this)" id="pwmSlider1" min="0" max="255" value="%SLIDER1VALUE%" step="1" class="slider"></p>
  <p><span id="textSlider2Value">Hue: %SLIDER2VALUE%</span></p>
  <p><input type="range" onchange="updateSlider2PWM(this)" id="pwmSlider2" min="0" max="255" value="%SLIDER2VALUE%" step="1" class="slider"></p>
<script>
function updateSlider1PWM(element) {
  var slider1Value = document.getElementById("pwmSlider1").value;
  document.getElementById("textSlider1Value").innerHTML = "Brightness: "+slider1Value;
  console.log(slider1Value);
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/slider1?value="+slider1Value, true);
  xhr.send();
}
function updateSlider2PWM(element) {
  var slider2Value = document.getElementById("pwmSlider2").value;
  document.getElementById("textSlider2Value").innerHTML = "Hue: "+slider2Value;
  console.log(slider2Value);
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/slider2?value="+slider2Value, true);
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if (var == "SLIDER1VALUE"){
    return slider1Value;
  } else if (var == "SLIDER2VALUE"){
    return slider2Value;
  }
  
  return String();
}

void processSlider1Change(int sliderVal) {
  LEDBrightness = sliderVal;
  FastLED.setBrightness(LEDBrightness);
}

void processSlider2Change(int sliderVal) {
  LEDHue = sliderVal;
}

// The Arduino map() function produces incorrect distribution for integers. See https://github.com/arduino/Arduino/issues/2466
// So, create a fixed function instead:
// map() maps a value of fromLow would get mapped to toLow, a value of fromHigh to toHigh, values in-between to values in-between, etc.
// syntax: map(value, fromLow, fromHigh, toLow, toHigh)
long map2(long x, long in_min, long in_max, long out_min, long out_max) {
        return (x - in_min) * (out_max - out_min+1) / (in_max - in_min+1) + out_min;
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.println(__FILE__ " Created:" __DATE__ " " __TIME__);
  
  // Init LED strip
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
//  FastLED.setBrightness(BRIGHTNESS); // set master brightness control
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
  FastLED.clear();
  
// process initial values to hardware
  processSlider1Change(slider1Value.toInt());
  processSlider2Change(slider2Value.toInt());

  // Connect to Wi-Fi
  WiFi.begin(SSID, WiFiPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/slider1?value=<inputMessage>
  server.on("/slider1", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider1?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      slider1Value = inputMessage;
      processSlider1Change(slider1Value.toInt());
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  // Send a GET request to <ESP_IP>/slider2?value=<inputMessage>
  server.on("/slider2", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input2 value on <ESP_IP>/slider2?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      slider2Value = inputMessage;
      processSlider2Change(slider2Value.toInt());
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });
  
  // Start server
  server.begin();
}
  
void loop() {
  for (int i=0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(LEDHue, LEDHue == 0 ? 0 : 255, 255);
  }
  
  FastLED.show();  
}
