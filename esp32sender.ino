#include <esp_now.h>
#include <WiFi.h>
#define SLEEP_TIME 1800
#define RELAY 13 //relay that turns on the moisture sensors
uint8_t slaveAddress[] = {0x30, 0x83, 0x98, 0xDB, 0xEE, 0xF0};
// Structure to keep the temperature and humidity data from a DHT sensor
typedef struct humidity {
  int id;
  float humidity;
};
// Create a struct_message called myData
humidity Data;
// Callback to have a track of sent messages
void OnSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nSend message status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sent Successfully" : "Sent Failed");
}
 
void setup() {
  // Init Serial Monitor
  Serial.begin(115200);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, HIGH);
 
  // Set device as a Wi-Fi Station
 WiFi.mode(WIFI_STA);
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("There was an error initializing ESP-NOW");
    return;
  }
  else{
    Serial.println("ESP-NOW initialized");
  }
  // We will register the callback function to respond to the event
  esp_now_register_send_cb(OnSent);
  
  // Register the slave
  esp_now_peer_info_t slaveInfo;
  memset(&slaveInfo, 0, sizeof(slaveInfo));
  memcpy(slaveInfo.peer_addr, slaveAddress, 6);
  slaveInfo.channel = 0;  
  slaveInfo.encrypt = false;
  
  // Add slave        
  if (esp_now_add_peer(&slaveInfo) != ESP_OK){
    Serial.println("There was an error registering the slave");
    return;
  }
  else{
    Serial.println("Slave registered");
  }
}
void loop() {
  digitalWrite(RELAY, LOW);
  delay(1000);
  for (int i = 32; i < 36; i = i + 1) {
   Data.humidity = map(analogRead(int(i)), 0, 4095, 100, 0);
   Data.id = i -31;
   Serial.print("Moisture Sensor Value:");
   Serial.print(Data.humidity);
   Serial.print(" id: ");
   Serial.println(Data.id);
  // Set values to send
  // To simplify the code, we will just set two floats and I'll send it 
  // Is time to send the messsage via ESP-NOW
  esp_err_t result = esp_now_send(slaveAddress, (uint8_t *) &Data, sizeof(Data));
   
  if (result == ESP_OK) {
    Serial.println("The message was sent sucessfully.");
  }
  else {
    Serial.println("There was an error sending the message.");
  }
  delay(2000);
}
Serial.print(SLEEP_TIME);
Serial.println(" seconds");
Serial.println("Going to sleep now");
esp_sleep_enable_timer_wakeup(SLEEP_TIME * 1000000);
esp_deep_sleep_start();
Serial.println("just got awake");
}
