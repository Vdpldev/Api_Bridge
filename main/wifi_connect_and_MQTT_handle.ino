#define TIMEOUT_DELAY 60000

String shiftFourDigits(String mac) 
{
    return mac.substring(4) + mac.substring(0, 4);
}

bool connectToWiFi() 
{
    blink100ms();
    String storedSSID = readEEPROM(0, 32);
    String storedPassword = readEEPROM(32, 32);
    
    String mac = WiFi.macAddress();
    mac.toLowerCase();
    mac.replace(":", "");
    String newMac = shiftFourDigits(mac);

    static char macCharWithPub[18];
    static char macCharWithSub[18];
    String macWithPub = newMac + "p";
    String macWithSub = newMac + "s";
    macWithPub.toCharArray(macCharWithPub, sizeof(macCharWithPub));
    macWithSub.toCharArray(macCharWithSub, sizeof(macCharWithSub));

    pubtopic = macCharWithPub;
    subtopic = macCharWithSub;

    if (storedSSID.length() > 0) 
    {
        DEBUG.println("Trying to connect to saved WiFi...");
        WiFi.mode(WIFI_STA);
        WiFi.begin(storedSSID.c_str(), storedPassword.c_str());

        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED) 
        {
            if (millis() - startTime >= TIMEOUT_DELAY) 
            {
                return false;
            }
            DEBUG.println(".");
            checkSerialWhileAP();  // ✅ allow serial input while waiting
            delay(10);             // 💡 small yield
        }

        DEBUG.println("\nConnected to WiFi.");
        blink400ms();
        digitalWrite(LED_PIN, HIGH);
        return true;
    }

    checkSerialWhileAP();  // always check serial even if no SSID
    return false;
}

void reconnectToMQTT() 
{
    if (!subtopic || !pubtopic) 
    {
        DEBUG.println("MQTT topics not set!");
        return;
    }

    if (WiFi.status() == WL_CONNECTED && !client.connected()) 
    {
        String clientId = "esp8266-client-" + WiFi.macAddress();
        DEBUG.println("Attempting MQTT connection...");

        if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) 
        {
            DEBUG.println("MQTT Connected!");
            client.subscribe(subtopic);
            client.publish(pubtopic, "MQTT Connected!");
            blinkoff();
        } 
        else 
        {
            DEBUG.print("MQTT connection failed, rc=");
            DEBUG.println(client.state());
            blink400ms();
            delay(5000);

        }
    }
}

void callback(char* subtopic, byte* payload, unsigned int length) 
{
    String message;
    for (unsigned int i = 0; i < length; i++) 
    {
        message += (char)payload[i];
    }

    DEBUG.println("Received MQTT Message: " + message);

    if (length < 2 || payload[1] == 0x40) 
    {
        checkAndUpdate();
        downloadAndUpdate();
    }
    else if (length < 2 || payload[1] == 0x41) 
    {
        checkAndUpdate();
        downloadAndUpdate();
    }
    else
    {
        Serial.print(message);
    }
}