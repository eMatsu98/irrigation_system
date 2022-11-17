// Import Wi-Fi library
#include <WiFi.h>
//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

#include <PubSubClient.h>

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Libraries to get time from NTP Server
#include <NTPClient.h>
#include <WiFiUdp.h>

//define the pins used by the LoRa transceiver module
#define SCLK 5
#define MISO 19
#define MOSI 27
#define CS 18
#define RST 23
#define DIO 26

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 433E6

//OLED pins
#define OLED_SDA 21
#define OLED_SCL 22 
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Replace with your network credentials
const char* ssid     = "Ana es la Mejor";
const char* password = "karlaana";

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String day;
String hour;
String timestamp;


// Initialize variables to get and save LoRa data
int rssi;
String loRaMessage;
String id;
String humidity;
String readingID;
long lastMsg = 0;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

#define mqtt_server "192.168.100.138" // our Home assitant Server
#define mqtt_user "homeassistant"
#define mqtt_password "via0iadee3jiepheishi1ahshuo7Noh3kaeyahzeosh9raix1Saroxuyi1ot2lee"

WiFiClient espClient;
PubSubClient client(espClient);

String temperature_topic;

// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if(var == "id"){
    return id;
  }
  else if(var == "HUMIDITY"){
    return humidity;
  }
  else if(var == "TIMESTAMP"){
    return timestamp;
  }
  else if (var == "RRSI"){
    return String(rssi);
  }
  return String();
}

//Initialize OLED display
void startOLED(){

  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("LORA SENDER");
}

//Initialize LoRa module
void startLoRA(){
  //SPI LoRa pins
  int counter;
  SPI.begin(SCLK, MISO, MOSI, CS);
  //setup LoRa transceiver module
  LoRa.setPins(CS, RST, DIO);

  while (!LoRa.begin(BAND) && counter < 10) {
    Serial.print(".");
    counter++;
    delay(500);
  }
  if (counter == 10) {
    // Increment readingID on every new reading
    Serial.println("Starting LoRa failed!"); 
  }
  Serial.println("LoRa Initialization OK!");
  display.setCursor(0,10);
  display.clearDisplay();
  display.print("LoRa Initializing OK!");
  display.display();
  delay(2000);
}

void connectWiFi(){
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  display.setCursor(0,20);
  display.print("Access web server at: ");
  display.setCursor(0,30);
  display.print(WiFi.localIP());
  display.display();
}

// Read LoRa packet and get the sensor readings
void getLoRaData() {
  Serial.print("Lora packet received: ");
  // Read packet
  while (LoRa.available()) {
    String LoRaData = LoRa.readString();
    // LoRaData format: readingID/temperature&soilMoisture#batterylevel
    // String example: 1/27.43&654#95.34
    Serial.print(LoRaData); 
    
    // Get readingID, temperature and soil moisture
    int pos2 = LoRaData.indexOf('&');
    int pos3 = LoRaData.indexOf('#');
    id = LoRaData.substring(0, pos2);
    humidity = LoRaData.substring(pos2+1, pos3);
  }
  // Get RSSI
  rssi = LoRa.packetRssi();
  Serial.print(" with RSSI ");    
  Serial.println(rssi);
  Serial.println("id:" + id);
  Serial.println("Temperature:" + humidity);
  #define temperature_topic  "sensor" + id + "/humidity"
  Serial.println(temperature_topic);
}

// Function to get date and time from NTPClient
void getTimeStamp() {
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();
  Serial.println(formattedDate);

  // Extract date
  int splitT = formattedDate.indexOf("T");
  day = formattedDate.substring(0, splitT);
  Serial.println(day);
  // Extract time
  hour = formattedDate.substring(splitT+1, formattedDate.length()-1);
  Serial.println(hour);
  timestamp = day + " " + hour;
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Connecting MQTT...");
    // Attempt to connect

    if (client.connect(mqtt_server, mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() { 
  // Initialize Serial Monitor
  Serial.begin(115200);
  startOLED();
  startLoRA();
  connectWiFi();
  // Initialize a NTPClient to get time
  timeClient.begin();
  timeClient.setTimeOffset(0);
  
  client.setServer(mqtt_server, 1883);
}

void loop() {
  // Check if there are LoRa packets available
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    getLoRaData();
    getTimeStamp();
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 1000) { // publish frequency
     lastMsg = now;
  client.publish(String(temperature_topic).c_str(), String(humidity).c_str(), true);
  }
}
