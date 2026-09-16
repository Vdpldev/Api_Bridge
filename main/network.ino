char mqtt_pub_topic[18];
char mqtt_sub_topic[18];

static String jsonPayload = "";
static bool receiving = false;
static unsigned long connectionStartTime = 0;

<<<<<<< HEAD
String shiftFourDigits(String mac){

    return mac.substring(4) + mac.substring(0, 4);
}

void generateMqttTopics(){

=======
String shiftFourDigits(String mac) 
{
    return mac.substring(4) + mac.substring(0, 4);
}


void generateMqttTopics() 
{
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
  memset(mqtt_pub_topic, 0, sizeof(mqtt_pub_topic));
  memset(mqtt_sub_topic, 0, sizeof(mqtt_sub_topic));

  String mac = WiFi.macAddress();
<<<<<<< HEAD
  mac.replace(":", "");
  mac.toLowerCase();
  if (mac.length() < 12){

=======

  mac.replace(":", "");
  mac.toLowerCase();

  if (mac.length() < 12) 
  {
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
    #ifdef DEBUG    
      Serial.println("[ERROR] MAC not ready. Retrying...");
    #endif
    mac = WiFi.macAddress();
    mac.replace(":", "");
  }

<<<<<<< HEAD
  if (mac.length() >= 12){

=======
  if (mac.length() >= 12) 
  {
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
    String scrambledMac = shiftFourDigits(mac) ;
    String pub = scrambledMac + "s" ;

    String sub = scrambledMac + "s";
    
<<<<<<< HEAD
    
=======
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
    pub.toCharArray(mqtt_pub_topic, sizeof(mqtt_pub_topic));
    sub.toCharArray(mqtt_sub_topic, sizeof(mqtt_sub_topic));

  } 
<<<<<<< HEAD
  else{

=======
  else 
  {
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
    strcpy(mqtt_pub_topic, "unknown_device_p");
    strcpy(mqtt_sub_topic, "unknown_device_s");
  }
  #ifdef DEBUG
    Serial.print("[NETWORK] Pub Topic: "); 
    Serial.println(mqtt_pub_topic);
  #endif    
}

<<<<<<< HEAD
void startWifiStation(const char* ssid, const char* pass){

=======
void startWifiStation(const char* ssid, const char* pass) 
{
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
  #ifdef DEBUG  
    Serial.println("[NETWORK] Connecting to WiFi...");
  #endif  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
}

<<<<<<< HEAD
int getWifiStatus(){
  return WiFi.status(); 
}

void startAPMode(){

=======
int getWifiStatus() 
{
  return WiFi.status(); 
}

void startAPMode() 
{
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
  #ifdef DEBUG  
    Serial.println("[NETWORK] Starting Access Point Mode...");
  #endif  
  WiFi.mode(WIFI_AP);

  String macAddress = WiFi.macAddress();
  macAddress.replace(":", "");
  String apName = "vdpltcp_" + macAddress;  

  WiFi.softAPConfig(apIP, apGateway, apSubnet);  
  WiFi.softAP(apName.c_str(), "12345678");  
  #ifdef DEBUG  
    Serial.print("[NETWORK] AP IP Address: "); 
    Serial.println(WiFi.softAPIP());
  #endif
  server.begin(); 
  #ifdef DEBUG  
    Serial.println("[TCP] Server Started...");
<<<<<<< HEAD
  #endif

}

void handleTcpConfig(){

=======
  #endif      
}

void handleTcpConfig() 
{
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
  WiFiClient client = server.available();
  if (!client) return; 

  #ifdef DEBUG
    Serial.println("[TCP] Client detected! Analyzing data...");
  #endif

  String incomingData = "";
  unsigned long timeout = millis();

<<<<<<< HEAD
  while (client.connected() && millis() - timeout < 3000){

    while (client.available()){

=======
  while (client.connected() && millis() - timeout < 3000) 
  {
    while (client.available()) 
    {
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
      char c = client.read();
      incomingData += c;
      timeout = millis(); 

      if (c == '}') goto parseData; 
    }
    yield();
  }

  parseData:
<<<<<<< HEAD
  if (incomingData.length() > 0){

=======
  if (incomingData.length() > 0) 
  {
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
    #ifdef DEBUG
      Serial.print("[TCP] Data Received: ");
      Serial.println(incomingData);
    #endif

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, incomingData);
    
<<<<<<< HEAD
    if (!error){

      String newSSID = doc["ssid"];
      String newPass = doc["password"];
      
      if (newSSID.length() > 0) {
=======
    if (!error) 
    {
      String newSSID = doc["ssid"];
      String newPass = doc["password"];

      if (newSSID.length() > 0) 
      {
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
        #ifdef DEBUG
          Serial.println("[TCP] Credentials Valid. Saving...");
        #endif
        
        saveCredentials(newSSID, newPass);
        generateMqttTopics();

        requestDeviceInfo();
        uint8_t mcuFrame[128];
        int frameLen = captureSerialResponse(mcuFrame, 128);
<<<<<<< HEAD

=======
         
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
        char clippedTopic[13]; 
        String mac = WiFi.macAddress();

        mac.replace(":", "");
        mac.toLowerCase();
        mac.toCharArray(clippedTopic, sizeof(mqtt_pub_topic));
      
        sendCombinedResponse(client, mcuFrame, frameLen, clippedTopic);
<<<<<<< HEAD
      
        Restart();
=======
        ESP.restart();
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
      }
    }
    #ifdef DEBUG
      else 
      {
        Serial.print("[TCP] JSON Error: ");
        Serial.println(error.c_str());
      }
    #endif
  }
}
<<<<<<< HEAD
void Restart(){
  currentState = ST_LOAD_CONFIG;
}
void sendCombinedResponse(WiFiClient& client, uint8_t* frame, int frameLen, char* topic){

=======

void sendCombinedResponse(WiFiClient& client, uint8_t* frame, int frameLen, char* topic) 
{
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
  uint8_t masterBuffer[TCP_BUFFER_SIZE]; 
  memset(masterBuffer, 0, TCP_BUFFER_SIZE);
  int totalLength = 0;

<<<<<<< HEAD
  if (frameLen > 0 && frameLen < (TCP_BUFFER_SIZE - 32)) {

=======
  if (frameLen > 0 && frameLen < (TCP_BUFFER_SIZE - 32)) 
  {
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
    memcpy(masterBuffer, frame, frameLen);
    totalLength = frameLen;
  }

  int topicLen = strlen(topic);
<<<<<<< HEAD
  if (totalLength + topicLen < TCP_BUFFER_SIZE) {

=======
  if (totalLength + topicLen < TCP_BUFFER_SIZE) 
  {
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
    memcpy(masterBuffer + totalLength, topic, topicLen);
    totalLength += topicLen;
  }

<<<<<<< HEAD
  if (totalLength > 0){

=======
  if (totalLength > 0) 
  {
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
    client.write(masterBuffer, totalLength);
    client.flush();
    #ifdef DEBUG
      Serial.printf("[TCP] Sent combined packet (%d bytes)\n", totalLength);
    #endif
  }
}