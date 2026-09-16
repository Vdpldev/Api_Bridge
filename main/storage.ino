#include <EEPROM.h>

<<<<<<< HEAD
void initStorage(){
  
=======
void initStorage() 
{
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
  EEPROM.begin(EEPROM_SIZE);
  #ifdef DEBUG  
    Serial.println("[STORAGE] EEPROM Initialized.");
  #endif  

  EEPROM.commit();  
}

<<<<<<< HEAD
void writeString(int addr, String data){
  
=======
void writeString(int addr, String data)
{
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
  for (int i = 0; i < data.length(); i++)
  {
    EEPROM.write(addr + i, data[i]);
  }
  EEPROM.write(addr + data.length(), '\0');
}

<<<<<<< HEAD
bool loadCredentials(){
  
  bool hasData = false;

  for (int i = 0; i < 32; i++) {
    
    deviceSettings.ssid[i] = EEPROM.read(ADDR_SSID + i);
  }
  
  for (int i = 0; i < 32; i++) {
    
    deviceSettings.password[i] = EEPROM.read(ADDR_PASS + i);
  }

  if (uint8_t(deviceSettings.ssid[0]) != 255 && deviceSettings.ssid[0] != 0) {
    
=======
bool loadCredentials() 
{
  bool hasData = false;

  for (int i = 0; i < 32; i++) 
  {
    deviceSettings.ssid[i] = EEPROM.read(ADDR_SSID + i);
  }
  
  for (int i = 0; i < 32; i++) 
  {
    deviceSettings.password[i] = EEPROM.read(ADDR_PASS + i);
  }

  if (uint8_t(deviceSettings.ssid[0]) != 255 && deviceSettings.ssid[0] != 0) 
  {
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
    hasData = true;
    #ifdef DEBUG    
      Serial.print("[STORAGE] Loaded SSID: ");
      Serial.println(deviceSettings.ssid);
    #endif
  } 
<<<<<<< HEAD
  else {
    
=======
  else 
  {
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
    #ifdef DEBUG    
      Serial.println("[STORAGE] No saved credentials found.");
    #endif  
  }

  return hasData;
}

<<<<<<< HEAD
void saveCredentials(String ssid, String pass){
  
  for (int i = 0; i < 32; i++) {

    EEPROM.write(ADDR_SSID + i, (i < ssid.length()) ? ssid[i] : 0);
  }
  for (int i = 0; i < 32; i++) {

=======
void saveCredentials(String ssid, String pass) 
{
  for (int i = 0; i < 32; i++) 
  {
    EEPROM.write(ADDR_SSID + i, (i < ssid.length()) ? ssid[i] : 0);
  }
  for (int i = 0; i < 32; i++) 
  {
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
    EEPROM.write(ADDR_PASS + i, (i < pass.length()) ? pass[i] : 0);
  }
  EEPROM.commit();
  #ifdef DEBUG  
    Serial.println("[STORAGE] New credentials saved to EEPROM.");
  #endif  
}

<<<<<<< HEAD
void clearEEPROM(){

=======
void clearEEPROM() 
{
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
  #ifdef DEBUG  
    Serial.println("[STORAGE] Clearing EEPROM... please wait.");
  #endif  
  
<<<<<<< HEAD
  for (int i = 0; i < EEPROM_SIZE; i++) {

    EEPROM.write(i, 0);
  }
  
  if (EEPROM.commit()) {

=======
  for (int i = 0; i < EEPROM_SIZE; i++) 
  {
    EEPROM.write(i, 0);
  }
  
  if (EEPROM.commit()) 
  {
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
    #ifdef DEBUG    
      Serial.println("[STORAGE] Memory wiped successfully.");
    #endif  
  } 
<<<<<<< HEAD
  else {

=======
  else 
  {
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
    #ifdef DEBUG    
      Serial.println("[STORAGE] ERROR: Failed to wipe memory.");
    #endif  
  }
}