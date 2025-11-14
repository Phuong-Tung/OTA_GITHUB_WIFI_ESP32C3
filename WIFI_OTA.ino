#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include "OTA.h"

// ===== Biến cấu hình =====
extern const unsigned long updateCheckInterval = 60 * 1000; // 1 phút

const char* ssid     = "ROVI HOLDINGS";
const char* password = "Rovi@4HG";

const char* firmwareUrl = "https://github.com/Phuong-Tung/OTA_GITHUB_WIFI_ESP32C3/releases/latest/download/WIFI_OTA_LED.ino.esp32c3.bin";
const char* versionUrl  = "https://raw.githubusercontent.com/Phuong-Tung/OTA_GITHUB_WIFI_ESP32C3/main/version.txt";
const char* currentFirmwareVersion = "1.0.1";
String g_targetVersion;

// ===== Khởi tạo =====
void setup() {
  Serial.begin(115200); 
  delay(500);
  Serial.println("\n[ESP32 FreeRTOS OTA]");

  // Task OTA nằm trong thư viện
  xTaskCreatePinnedToCore(OtaTask, "OtaTask", 12288, nullptr, 1, nullptr, 0);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
