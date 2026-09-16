#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient mqttClient(espClient);

<<<<<<< HEAD
void mqttCallback(char* topic, byte* payload, unsigned int length) {

=======
void mqttCallback(char* topic, byte* payload, unsigned int length) 
{
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
  String msg = "";
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  #ifdef DEBUG
    Serial.print("[MQTT] Message arrived on topic: ");
    Serial.println(topic);
  #endif

<<<<<<< HEAD
  if (msgQueue.size() < 10) {
    msgQueue.push_back(msg);
  }
}

void initMqtt(){

=======
  if (msgQueue.size() < 10) 
  {
    msgQueue.push(msg);
  }
}

void initMqtt() 
{
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
}

<<<<<<< HEAD
bool attemptMqttConnect(){

  if (!mqtt_pub_topic || !mqtt_sub_topic) {

=======
bool attemptMqttConnect() 
{
  if (!mqtt_pub_topic || !mqtt_sub_topic) 
  {
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
    #ifdef DEBUG
      Serial.println("MQTT topics not set!");
    #endif

    return false;
  }
<<<<<<< HEAD
  if (WiFi.status() == WL_CONNECTED && !mqttClient.connected()){

    String clientId = "esp8266-client-" + WiFi.macAddress();
    
=======
  if (WiFi.status() == WL_CONNECTED && !mqttClient.connected())
  {
    String clientId = "esp8266-client-" + WiFi.macAddress();

>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
    #ifdef DEBUG
      Serial.println("[MQTT] Attempting connection...");
    #endif

<<<<<<< HEAD
    if (mqttClient.connect(clientId.c_str(), mqtt_username, mqtt_password)){
      
=======
    if (mqttClient.connect(clientId.c_str(), mqtt_username, mqtt_password))
    {
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
      #ifdef DEBUG    
        Serial.println("[MQTT] Connected!");
      #endif

      mqttClient.subscribe(mqtt_sub_topic);
      publishToCloud("MQTT Connected!");
      
      return true;
    }
  }
  return false;
}

<<<<<<< HEAD
void processMqtt(){

  mqttClient.loop();
}

void publishToCloud(const char* message) {

  if (mqttClient.connected()) {

=======
void processMqtt() 
{
  mqttClient.loop();
}

void publishToCloud(const char* message) 
{
  if (mqttClient.connected()) 
  {
>>>>>>> 13bb0663cbc367c3f5669d25100a7fcce62a0dfa
    mqttClient.publish(mqtt_pub_topic, message);
  }
}