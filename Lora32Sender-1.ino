#include <SPI.h>
#include <LoRa.h>

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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

//Libraries from ESPNOW
#include <esp_now.h>
#include <WiFi.h>


int counter = 0;
String LoRaMessage = "";

int idread = 0;
float humidityread = 0;


// Structure to keep the temperature and humidity data
// Is also required in the client to be able to save the data directly
typedef struct humidity {
  int id;
  float humidity;
};
// Create a struct_message called myData
humidity Data;
// callback function executed when data is received
void OnRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&Data, incomingData, sizeof(Data));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Humidity: ");
  Serial.println(Data.humidity);
  Serial.print("id: ");
  Serial.println(Data.id);
}

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

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

void getReadings(){
  idread = Data.id;
  humidityread = Data.humidity;
}

void sendReadings() {
  LoRaMessage = String(idread) + "&" + String(humidityread);
  //Send LoRa packet to receiver
  LoRa.beginPacket();
  LoRa.print(LoRaMessage);
  LoRa.endPacket();
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.print("LoRa packet sent!");
  display.setCursor(0,20);
  display.print("id:");
  display.setCursor(72,20);
  display.print(idread);
  display.setCursor(0,30);
  display.print("Humidity:");
  display.setCursor(54,30);
  display.print(humidityread);
  display.setCursor(0,50);
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("There was an error initializing ESP-NOW");
    return;
  }

  startOLED();
  startLoRA();
  
  // Once the ESP-Now protocol is initialized, we will register the callback function
  // to be able to react when a package arrives in near to real time without pooling every loop.
  esp_now_register_recv_cb(OnRecv);
}
void loop() {
  getReadings();
  sendReadings();
  delay(1000);
}
