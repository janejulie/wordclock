#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>
#include <FS.h>
#include <WebSocketsServer.h>

const int BUILT_IN_LED_ESP = 2;
const int BUILT_IN_LED_NODE = 16;

const char* ssid     = "Wifi name";     //change
const char* password = "Wifi password"; //change
ESP8266WiFiMulti wifiMulti;
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);
File fsUploadFile;
const char* mdnsName = "wordclock";

const char* NTP_SERVER  = "0.de.pool.ntp.org";
const char* TIMEZONE    = "CET-1CEST,M3.5.0/02,M10.5.0/03";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

const int CLOCK_STATE = 1;
int state = CLOCK_STATE;

// pixels
const int PIN = 4;
const int NUMPIXELS = 114;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_RGB + NEO_KHZ800);
const uint32_t WHITE_LIGHT = pixels.Color(255, 255, 255);
const uint32_t RED_LIGHT = pixels.Color(0, 255, 0);
const uint32_t BLUE_LIGHT = pixels.Color(0, 0, 255);
const uint32_t GREEN_LIGHT = pixels.Color(255, 0, 0);
const uint32_t OFF = pixels.Color(0, 0, 0);
const uint32_t colors[] = {WHITE_LIGHT, RED_LIGHT, BLUE_LIGHT, GREEN_LIGHT, OFF};

int ledHours[][7] = {{108, 99, 88, 79, -1, -1, -1}, //1
  {39, 28, 19, 8, -1, -1, -1}, //2
  {107, 100, 87, 80, -1, -1, -1}, //3
  {40, 27, 20, 7, -1, -1, -1}, //4
  {38, 29, 18, 9, -1, -1, -1}, //5
  {106, 101, 86, 81, 66, -1, -1},  //6
  {105, 102, 85, 82, 65, 62, -1},  //7
  {41, 26, 21, 6, -1, -1, -1},  //8
  {83, 64, 63, 44, -1, -1, -1},  //9
  {104, 103, 84, 83, -1, -1, -1},  //10
  {58, 49, 38, -1, -1, -1, -1},  //11
  {45, 42, 25, 22, 5, -1, -1} //12
};

int es[] = {113, 94, -1, -1, -1, -1, -1};
int ist[] = {74, 73, 54, -1, -1, -1, -1};
int uhr[] = {24, 23, 4, -1, -1, -1, -1};
int vor[] = {110, 97, 90, -1, -1, - 1, -1};
int nach[] = {37, 30, 17, 10, -1, -1, -1};

int fuenf[] = {34, 33, 14, 13, -1, -1, -1};
int zehn[] = {112, 95, 92, 75, -1, -1 , -1};
int viertel[] = {71, 56, 51, 36, 31, 16, 11};
int zwanzig[] = {72, 55, 52, 35, 32, 15, 12};
int halb[] = {109, 98, 89, 78, -1 , -1, -1};

int currentColor = 0;

// function prototypes
void handleRoot();
void handleLED();
void handleNotFound();
void handleTurnOnAll();
void handleColorChange();
void getTimeFromNTPServer();
String getContentType(String filename);
bool handleFileRead(String path);

time_t now;
tm tm;

void setup(void) {
  pinMode (PIN, OUTPUT);
  pinMode(BUILT_IN_LED_ESP, OUTPUT);
  pinMode(BUILT_IN_LED_NODE, OUTPUT);
  digitalWrite(BUILT_IN_LED_ESP, LOW);
  digitalWrite(BUILT_IN_LED_NODE, LOW);

  Serial.begin(115200);
  startTime();
  startWifi();
  startSPIFFS();
  startMDNS();
  startWebSocket();
  startServer();
  startPixels();

}

void loop(void) {
  webSocket.loop();
  server.handleClient();
  MDNS.update();
  getTimeFromNTPServer();
  if (state == CLOCK_STATE) {
    showClock();
  }
}

void startTime(void) {
  configTime(TIMEZONE, NTP_SERVER);
  timeClient.begin();
}

void startWifi(void) {
  wifiMulti.addAP(ssid, password);
  Serial.println("Connecting ...");
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(250);
    Serial.print('.');
  }
  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
}

void startSPIFFS() { // Start the SPIFFS and list all contents
  SPIFFS.begin();                             // Start the SPI Flash File System (SPIFFS)
  Serial.println("SPIFFS started. Contents:");
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {                      // List the file system contents
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
}

void startMDNS() { // Start the mDNS responder
  MDNS.begin(mdnsName);                        // start the multicast domain name server
  Serial.print("mDNS responder started: http://");
  Serial.print(mdnsName);
  Serial.println(".local");
}

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

void startServer() { // Start a HTTP server with a file read handler and an upload handler
  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
  server.begin();                             // start the HTTP server
  Serial.println("HTTP server started.");
}

void startPixels() {
  pixels.begin();
}

String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

bool handleFileRead(String path) {
  if (path.endsWith("/")) path += "index.html";
  String contentType = getContentType(path);
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  Serial.println("\tFile Not Found");
  return false;
}

void handleNotFound() {
  if (!handleFileRead(server.uri())) {
    server.send(404, "text/plain", "404: File Not Found");
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT: {
        Serial.println(payload[0]);
        if (payload[0] == 'W') {
          wordclock();
        } else if (payload[0] == 'X') {
          changeColor();
        } else if (payload[0] == 'A') {
          turnOnAll();
        } else if (payload[0] == 'N') {
          turnOffAll();
        } else if (payload[0] == 'L') {
          toggleLed();
        }
      }
      break;
  }
}

void changeColor() {
  currentColor = (currentColor + 1) % 4;
}
void turnOnAll() {
  if (currentColor == 5) {
    currentColor = 0;
  }
  state = 0;
  changeAll();
}

void turnOffAll() {
  state = 0;
  currentColor = 5;
  changeAll();
}

void wordclock() {
  if (currentColor == 5) {
    currentColor = 0;
  }
  state = CLOCK_STATE;
}

void toggleLed() {
  digitalWrite(BUILT_IN_LED_ESP, !digitalRead(BUILT_IN_LED_ESP));
}

void showClock() {
  pixels.clear();
  int h = (((tm.tm_hour + 11) % 12));
  int m = tm.tm_min;

  turnOnArray(es);
  turnOnArray(ist);
  if (m < 5) {
    pixels.setPixelColor(79, OFF); // einS
    turnOnArray(uhr); // UHR
  } else if (m < 10) {
    turnOnArray(fuenf);
    turnOnArray(nach);
  } else if (m < 15) {
    turnOnArray(zehn);
    turnOnArray(nach);
  } else if (m < 20) {
    turnOnArray(viertel);
    turnOnArray(nach);
  } else if (m < 25) {
    turnOnArray(zwanzig);
    turnOnArray(nach);
  } else if (m < 30) {
    h = (h + 1) % 12;
    turnOnArray(fuenf);
    turnOnArray(vor);
    turnOnArray(halb);
  } else if (m < 35) {
    h = (h + 1) % 12;
    turnOnArray(halb);
  } else if (m < 40) {
    h = (h + 1) % 12;
    turnOnArray(fuenf);
    turnOnArray(nach);
    turnOnArray(halb);
  } else if (m < 45) {
    h = (h + 1) % 12;
    turnOnArray(zwanzig);
    turnOnArray(vor);
  } else if (m < 50) {
    h = (h + 1) % 12;
    turnOnArray(viertel);
    turnOnArray(vor);
  } else if (m < 55) {
    h = (h + 1) % 12;
    turnOnArray(zehn);
    turnOnArray(vor);
  } else if (m < 60) {
    h = (h + 1) % 12;
    turnOnArray(fuenf);
    turnOnArray(vor);
  } else {
    Serial.print("Whut?");
  }

  turnOnArray(ledHours[h]);

  for (int j = 0; j < (m % 5); j++) {
    pixels.setPixelColor(j, colors[currentColor]);
  }
  pixels.show();
}

void changeAll() {
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, colors[currentColor]);
  }
  pixels.show();
}

void turnOnArray(int* leds) {
  for (int j = 0; j < 7; j++) {
    pixels.setPixelColor(leds[j], colors[currentColor]);
  }
}

String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}

void getTimeFromNTPServer() {
  time(&now);                       // read the current time
  localtime_r(&now, &tm);           // update the structure tm with the current time
}
