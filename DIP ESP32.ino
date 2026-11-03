



#include <WiFi.h>
#include <HTTPClient.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// --- 1. CHANGE THESE TO YOUR ACTUAL WI-FI CREDENTIALS ---
const char* ssid = "Yk";
const char* password = "harrypotter";

// --- 2. CHANGE THIS TO YOUR GOOGLE WEB APP URL ---
String webAppUrl = "https://script.google.com/macros/s/AKfycbwPhb8eTR5EZkNEjdGxeeUhfiH9k6RVfHRYBIdwrjgunr_f1lTCR8zNmlqqUcXaGDOaEw/exec"; 

BLEScan* pBLEScan;
String latestData = "";
bool newDataAvailable = false;

// This callback triggers when the Seeed XIAO broadcasts data
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      if (advertisedDevice.getName() == "DIP_NODE") {
        String strData = advertisedDevice.getManufacturerData();
        if (strData.length() > 2) {
            // Extract the payload (e.g., "0.00,0.00")
            latestData = strData.substring(2); 
            newDataAvailable = true;
            Serial.println("📡 Caught New Data: " + latestData);
        }
      }
    }
};

void setup() {
  Serial.begin(115200);

  // Connect Wi-Fi
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Wi-Fi Connected!");

  // Start BLE Scanner
  Serial.println("Starting BLE Scanner...");
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); 
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  
}

void loop() {
  // 1. Scan the room for the Seeed XIAO for 3 seconds
  pBLEScan->start(3, false);
  pBLEScan->clearResults(); 

  // 2. If we caught new data, push it to Google Sheets
  if (newDataAvailable && WiFi.status() == WL_CONNECTED) {
    
    // Split the comma-separated data ("0.00,0.00") into two variables
    int commaIndex = latestData.indexOf(',');
    if (commaIndex > 0) {
      String temp = latestData.substring(0, commaIndex);
      String lux = latestData.substring(commaIndex + 1);

      // Construct the secure Google URL
      String finalUrl = webAppUrl + "?temp=" + temp + "&lux=" + lux;

      // Send the data over Wi-Fi
      HTTPClient http;
      http.begin(finalUrl);
      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); // Required for Google
      
      int httpCode = http.GET();

      if (httpCode > 0) {
        Serial.println("☁️ Pushed to Google Sheets! Code: " + String(httpCode));
      } else {
        Serial.println("❌ Failed to push. Error: " + String(httpCode));
      }
      http.end();
    }
    
    // Reset the flag so we don't upload the exact same reading multiple times
    newDataAvailable = false;
    
    // Wait 15 seconds before scanning again so we don't spam Google's servers
    delay(15000); 
  } else {
    // Short wait if no data was found before trying again
    delay(1000);
  }
}