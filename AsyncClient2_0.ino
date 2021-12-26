/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp8266-nodemcu-websocket-server-arduino/
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

// Import required libraries
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Replace with your network credentials
//const char* ssid = "TitanDragon";
//const char* password = "L@tteMacchiat0!";
const char* ssid = "lamp";
const char* password = "lampjelampje!";

bool timerState = 0;
const int outPin = 2;
//Rolys global vars
String sliderValue = "600";
int countDown = 600;
int updateOnOff = 0;

const char* PARAM_INPUT = "value";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
  html {
    font-family: Arial, Helvetica, sans-serif;
    text-align: center;
  }
  h1 {
    font-size: 1.8rem;
    color: white;
  }
  h2{
    font-size: 1.5rem;
    font-weight: bold;
    color: #143642;
  }
  .topnav {
    overflow: hidden;
    background-color: #143642;
  }
  body {
    margin: 0;
  }
  .content {
    padding: 30px;
    max-width: 600px;
    margin: 0 auto;
  }
  .card {
    background-color: #F8F7F9;;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
    padding-top:10px;
    padding-bottom:20px;
  }
  .button {
    padding: 15px 50px;
    font-size: 24px;
    text-align: center;
    outline: none;
    color: #fff;
    background-color: #0f8b8d;
    border: none;
    border-radius: 5px;
    -webkit-touch-callout: none;
    -webkit-user-select: none;
    -khtml-user-select: none;
    -moz-user-select: none;
    -ms-user-select: none;
    user-select: none;
    -webkit-tap-highlight-color: rgba(0,0,0,0);
   }
   /*.button:hover {background-color: #0f8b8d}*/
   .button:active {
     background-color: #0f8b8d;
     box-shadow: 2 2px #CDCDCD;
     transform: translateY(2px);
   }
   .state {
     font-size: 1.5rem;
     color:#8c8c8c;
     font-weight: bold;
   }
  </style>
<title>Light switch timer interval</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
</head>
<body>
  <div class="topnav">
    <h1>Light switch timer interval</h1>
  </div>
  <div class="content">
    <div class="card">
      <h2>Output - Relay (GPIO2)</h2>
      <p class="state">Interval (s): <span id="textSliderValue">%SLIDERVALUE%</span></p>
      <p><input type="range" onchange="updateSlider(this)" id="timerSlider" min="10" max="1200" value="%SLIDERVALUE%" step="10" class="slider"></p>
      <p class="state">Counter: <span id="counter">%SLIDERVALUE%</span></p>
    </div>
  </div>
<script>
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log('Connection opened');
  }
  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  function onMessage(event) {
    var state;
    if (event.data == "ON"){
      state = "ON";
      document.getElementById('state').innerHTML = state;
    }
    else if (event.data == "OFF"){
      state = "OFF";
      document.getElementById('state').innerHTML = state;
    }
    else {
      document.getElementById('counter').innerHTML = event.data; 
    }
  }
  function onLoad(event) {
    initWebSocket();
    initButton();
  }
  function initButton() {
    document.getElementById('button').addEventListener('click', toggle);
  }
  function OnOff(){
    websocket.send('toggle');
  }
  function updateSlider(element) 
  {
   var sliderValue = document.getElementById("timerSlider").value;
   document.getElementById("textSliderValue").innerHTML = sliderValue;
   console.log(sliderValue);
   var xhr = new XMLHttpRequest();
   xhr.open("GET", "/slider?value="+sliderValue, true);
   xhr.send();
  }
</script>
</body>
</html>
)rawliteral";

// Interrupt Service Routine
void ICACHE_RAM_ATTR onTimerISR(){
    ws.cleanupClients();
    timer1_write(5000000);//1s
    countDown--;
    notifyClients();
    Serial.println(countDown);
    if (countDown == 0){
      digitalWrite(outPin,!(digitalRead(outPin)));  //Toggle Output Pin
      ws.cleanupClients();
      countDown = sliderValue.toInt();
      //Serial.println("Timer reached zero:" + countDown);
    }
    else{
//      if(globalClient != NULL && globalClient->status() == WS_CONNECTED){
//        Serial.println(countDown);
//        globalClient->text(String(countDown));
//        ws.textAll(String(countDown));
//      }
    }
}

void notifyClients() {
  Serial.println(String(updateOnOff) + " " + String(updateOnOff));
  if (updateOnOff == 1){ //timerStatus containd button change
    if (timerState ==0){
      ws.textAll("OFF");
      updateOnOff = 0;
      }
    else {
      ws.textAll("ON");
      updateOnOff = 0;
      } 
    }  
  else {
    ws.textAll(String(countDown));
  }

}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle") == 0) {
      timerState = !timerState;
      updateOnOff = 1; 
      notifyClients();
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
    switch (type) {
      case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
      case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
      case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
      case WS_EVT_PONG:
      case WS_EVT_ERROR:
        break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if (timerState){
      return "ON";
    }
    else{
      return "OFF";
    }
      if (var == "SLIDERVALUE"){
    return sliderValue;
    }
   if (var == "COUNTERVALUE"){
     return String(countDown);
   }
  }
  return String();
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  pinMode(outPin, OUTPUT);
  digitalWrite(outPin, HIGH);
  
  // Connect to Wi-Fi
//  WiFi.begin(ssid, password);
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(1000);
//    Serial.println("Connecting to WiFi..");
//  }
//
//  // Print ESP Local IP Address
//  Serial.println(WiFi.localIP());

  Serial.print("Setting soft-AP ... ");
  boolean result = WiFi.softAP(ssid, password);
  if(result == true)
  {
    Serial.println("Ready");
  }
  else
  {
    Serial.println("Failed!");
  }

  initWebSocket();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/slider?value=<inputMessage>
  server.on("/slider", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      sliderValue = inputMessage;
      countDown = inputMessage.toInt();
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  // Start server
  server.begin();

  //Initialize Ticker every 1s
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  timer1_write(5000000); //1 s

}

void loop() {
  //ws.cleanupClients(); /// Needs to be removed #Roly
  //digitalWrite(outPin, timerState);
}
