#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>

WiFiClientSecure httpsClient;

void initOta() 
{
  httpsClient.setInsecure();
}

bool checkForUpdates() 
{
  #ifdef DEBUG  
    Serial.println("[OTA] Checking server for new version...");
  #endif

  return true; 
}

void performUpdate() 
{
  #ifdef DEBUG  
    Serial.println("[OTA] Starting Download...");
  #endif

  t_httpUpdate_return ret = ESPhttpUpdate.update(httpsClient, OTA_URL, CURRENT_VERSION);

  switch (ret) 
  {
    case HTTP_UPDATE_FAILED:
      #ifdef DEBUG
        Serial.printf("[OTA] Update failed. Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      #endif
    break;

    case HTTP_UPDATE_NO_UPDATES:
      #ifdef DEBUG    
        Serial.println("[OTA] No updates available.");
      #endif      
    break;
    
    case HTTP_UPDATE_OK:
      #ifdef DEBUG    
        Serial.println("[OTA] Update successful! Rebooting...");
      #endif
    break;
  }
}