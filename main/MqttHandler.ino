#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void mqttCallback(char* topic, byte* payload, unsigned int length) 
{
  String msg = "";
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  #ifdef DEBUG
    Serial.print("[MQTT] Message arrived on topic: ");
    Serial.println(topic);
  #endif

  if (msgQueue.size() < 10) 
  {
    msgQueue.push(msg);
  }
}

void initMqtt() 
{
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
}

bool attemptMqttConnect() 
{
  if (!mqtt_pub_topic || !mqtt_sub_topic) 
  {
    #ifdef DEBUG
      Serial.println("MQTT topics not set!");
    #endif

    return false;
  }
  if (WiFi.status() == WL_CONNECTED && !mqttClient.connected())
  {
    String clientId = "esp8266-client-" + WiFi.macAddress();

    #ifdef DEBUG
      Serial.println("[MQTT] Attempting connection...");
    #endif

    if (mqttClient.connect(clientId.c_str(), mqtt_username, mqtt_password))
    {
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

void processMqtt() 
{
  mqttClient.loop();
}

void publishToCloud(const char* message) 
{
  if (mqttClient.connected()) 
  {
    mqttClient.publish(mqtt_pub_topic, message);
  }
}