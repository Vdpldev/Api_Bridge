// ✅ Start AP Mode & Enable TCP Server with Static IP
void startAPMode() {
    isAPMode = true;  // Track AP mode status
    lastWiFiRetry = millis();

    String macAddress = WiFi.macAddress();
    macAddress.replace(":", "");
    String apSSID = "vdpltcp_" + macAddress;

    DEBUG.println("Starting AP Mode...");
    WiFi.mode(WIFI_AP);
    
    // ✅ Set Static IP for AP mode
    WiFi.softAPConfig(apIP, apGateway, apSubnet);
    WiFi.softAP(apSSID.c_str(), "12345678");  // Default AP password

    DEBUG.println("AP Mode Started: " + apSSID);
    DEBUG.println("TCP Server IP for Client: 192.168.4.1");

    server.begin();  // ✅ Start TCP server in AP Mode only
    if (Serial.available() > 0) {
            byte incomingByte = Serial.read();
            if (bufferIndex < bufferSize) {
                buffer[bufferIndex++] = incomingByte;
            }
            // Check if the buffer contains the reset command hex frame
              if (isResetCommand(buffer, bufferIndex)) {
                handleReceivedHexData();
                bufferIndex = 0; // Clear the buffer index after processing
              }
    }
}

void tcpserver() {
    WiFiClient client = server.available();
    if (!client) return;

    DEBUG.println("Client Connected");
    String data = "";
    bool receiving = false;
    bool validDataReceived = false;

    String mac = WiFi.macAddress();
    mac.toLowerCase();
    mac.replace(":", "");

    while (client.connected()) {
        while (client.available()) {
            char c = client.read();
            if (c == '{') {
                data = "";
                receiving = true;
            }
            if (receiving) {
                data += c;
            }
            if (c == '}') {
                receiving = false;
                validDataReceived = true;
                break;
            }
        }
        if (validDataReceived) break;
    }

    if (!validDataReceived) return;

    DeserializationError error = deserializeJson(doc, data);
    if (error) return;

    String newSSID = doc["ssid"];
    String newPassword = doc["password"];
    if (newSSID.length() > 0 && newPassword.length() > 0) {
        saveCredentials(newSSID, newPassword);
        DEBUG.println("Saved SSID: " + newSSID);
        DEBUG.println("Saved Password: " + newPassword);

        // Call device info function
        gatedeviceinfo();
        delay(500);

        // --- Read raw serial bytes until '}' ---
        const size_t BUF_SIZE = 256;
        uint8_t buffer[BUF_SIZE];
        size_t index = 0;
        unsigned long start = millis();

        while (Serial.available()) {
            uint8_t b = Serial.read();
            if (index < BUF_SIZE){
                buffer[index++] = b;
            }

            if (b == '}') {
                break;
            }
            if (index >= BUF_SIZE) break;
        }

        if(index + mac.length() < BUF_SIZE){
            for(int i = 0; i < mac.length(); i++){
                buffer[index++] = mac.charAt(i);
            }
        }
    // --- Send raw serial data directly ---
        DEBUG.println("Forwarding RAW Serial Data to Client...");
        client.write(buffer, index);   // raw bytes


        // // Send MAC afterwards as readable text
        // client.print(mac);
        delay(1000);
        ESP.restart();
    }
}

