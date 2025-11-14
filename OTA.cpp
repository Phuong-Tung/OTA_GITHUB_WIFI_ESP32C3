#include "OTA.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

// --- Lấy cấu hình/biến từ .ino ---
extern const char* ssid;
extern const char* password;
extern const char* firmwareUrl;
extern const char* versionUrl;
extern const char* currentFirmwareVersion;
extern const unsigned long blinkInterval;
extern String g_targetVersion;

// =====================================================================
// FreeRTOS Task: WiFi + Blink + OTA (đã chuyển vào thư viện)
// =====================================================================
void OtaTask(void* pv) {
  unsigned long lastUpdate = 0;

  for (;;) {
    // ----- Kết nối WiFi -----
    if (WiFi.status() != WL_CONNECTED) {
      Serial.print("Connecting to WiFi...");
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);
      int retry = 0;
      while (WiFi.status() != WL_CONNECTED && retry < 20) {
        Serial.print(".");
        vTaskDelay(pdMS_TO_TICKS(500));
        retry++;
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

        // ✅ Kiểm tra ngay sau khi có WiFi
        Serial.println("Checking for firmware update...");
        String latest = fetchLatestVersion_();
        if (latest.length()) {
          latest.trim();
          Serial.println("Current: " + String(currentFirmwareVersion));
          Serial.println("Latest : " + latest);
          if (latest != currentFirmwareVersion) {
            g_targetVersion = latest;
            Serial.println("New firmware found → starting OTA...");
            downloadAndApplyFirmware_();
          } else {
            Serial.println("Already up to date.");
          }
        } else {
          Serial.println("Failed to fetch latest version");
        }
        lastUpdate = millis();  // mốc cho lần kế tiếp
      } else {
        Serial.println("\nWiFi connect failed, retrying...");
        vTaskDelay(pdMS_TO_TICKS(3000));
        continue;
      }
    }

    // ----- Kiểm tra OTA định kỳ -----
    if (millis() - lastUpdate >= updateCheckInterval && WiFi.status() == WL_CONNECTED) {
      lastUpdate = millis();
      Serial.println("Checking for firmware update...");
      String latest = fetchLatestVersion_();
      if (latest.length()) {
        latest.trim();
        Serial.println("Current: " + String(currentFirmwareVersion));
        Serial.println("Latest : " + latest);
        if (latest != currentFirmwareVersion) {
          g_targetVersion = latest;
          Serial.println("New firmware found → starting OTA...");
          downloadAndApplyFirmware_();
        } else {
          Serial.println("Already up to date.");
        }
      } else {
        Serial.println("Failed to fetch latest version");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// =====================================================================
// OTA functions
// =====================================================================
String fetchLatestVersion_() {
  HTTPClient http;
  http.begin(versionUrl);
  int code = http.GET();
  if (code == HTTP_CODE_OK) {
    String v = http.getString();
    v.trim();
    http.end();
    return v;
  }
  Serial.printf("HTTP error: %d\n", code);
  http.end();
  return "";
}

void downloadAndApplyFirmware_() {
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(firmwareUrl);
  int code = http.GET();
  Serial.printf("HTTP GET code: %d\n", code);

  if (code == HTTP_CODE_OK) {
    int len = http.getSize();
    Serial.printf("Firmware size: %d bytes\n", len);
    if (len > 0) {
      WiFiClient* stream = http.getStreamPtr();
      if (startOTAUpdate_(stream, len)) {
        Serial.printf("OTA update success: %s -> %s\n",
                      currentFirmwareVersion,
                      g_targetVersion.c_str());
        Serial.println("Restarting in 3s...");
        for (int i = 3; i > 0; --i) { Serial.printf("%d...\n", i); delay(500); }
        Serial.flush();
        ESP.restart();
      } else {
        Serial.println("OTA update failed");
      }
    } else {
      Serial.println("Invalid firmware size");
    }
  } else {
    Serial.printf("Failed to fetch firmware. HTTP code: %d\n", code);
  }

  http.end();
}

// =====================================================================
// Helper formatting
// =====================================================================
String fmtSpeed_(double bps) {
  if (bps < 1024.0) return String((int)bps) + " B/s";
  else if (bps < 1024.0 * 1024.0) return String(bps / 1024.0, 1) + " KB/s";
  else return String(bps / (1024.0 * 1024.0), 2) + " MB/s";
}

String fmtETA_(double s) {
  if (s < 0) s = 0;
  uint32_t sec = (uint32_t)(s + 0.5);
  char buf[16];
  snprintf(buf, sizeof(buf), "%02u:%02u", sec / 60, sec % 60);
  return String(buf);
}

// =====================================================================
// OTA write core
// =====================================================================
bool startOTAUpdate_(WiFiClient* client, int len) {
  Serial.println("Starting OTA...");
  if (!Update.begin(len)) {
    Serial.printf("Update begin failed: %s\n", Update.errorString());
    return false;
  }

  size_t written = 0;
  const int barWidth = 40;
  unsigned long start = millis(), last = start;

  while (written < (size_t)len) {
    if (client->available()) {
      uint8_t buf[1024];
      size_t r = client->read(buf, sizeof(buf));
      if (r > 0) {
        size_t w = Update.write(buf, r);
        written += w;

        unsigned long now = millis();
        if (now - last > 500) {
          int progress = (written * 100) / len;
          Serial.printf("\n[%.*s%*s] %3d%%",
                        (progress * barWidth) / 100, "========================================",
                        barWidth - (progress * barWidth) / 100, "",
                        progress);
          last = now;
        }
      }
    } else {
      vTaskDelay(pdMS_TO_TICKS(1));
    }
  }

  Serial.println();
  if (!Update.end()) {
    Serial.printf("Update failed: %s\n", Update.errorString());
    return false;
  }

  Serial.printf("Done in %.2f sec\n", (millis() - start) / 1000.0);
  return true;
}
