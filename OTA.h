#pragma once
#include <Arduino.h>
#include <WiFiClient.h>

// Khai báo biến toàn cục từ .ino
extern const unsigned long updateCheckInterval;

extern const char* ssid;
extern const char* password;
extern const char* firmwareUrl;
extern const char* versionUrl;
extern const char* currentFirmwareVersion;
extern String g_targetVersion;

// Hàm task & OTA
void OtaTask(void* pv);
String fetchLatestVersion_();
void   downloadAndApplyFirmware_();
bool   startOTAUpdate_(WiFiClient* client, int contentLength);
String fmtSpeed_(double bps);
String fmtETA_(double seconds);
