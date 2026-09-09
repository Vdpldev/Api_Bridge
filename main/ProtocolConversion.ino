

byte OEMBuffer[64];
byte calcChecksum(byte *buf, int len)
{
    byte chk = 0;

    for(int i = 1; i < len; i++)
    {
        chk += buf[i];
    }

    return chk;
}

byte* TuyaToOem(byte ver ,byte cmd , byte *tuyaData, int tuyaLen,int *oemLen){
    
    //const byte f[]={0x7B, 0x05, 0x03, 0x01, 0x01, 0x05, 0x7D}; Serial.write(f, 7);
    byte dpid;
    byte type;

    int idx = 0;
    int out = 0;

    dpid = tuyaData[idx++];
    type = tuyaData[idx++];

    uint16_t dataLen =
        ((uint16_t)tuyaData[idx] << 8) |
         tuyaData[idx + 1];
    
    
    OEMBuffer[out++] = OEM_START_BYTE;      // Start
    idx += 2;

    
    if(cmd == TUYA_CMD_REPORT_STATUS ){

        byte* dData = &tuyaData[idx];

        // Update our struct
        updateDeviceState(dpid, type, dData, dataLen);
        #ifdef DEBUG  
                printCurrentStatus("TUYA_MCU");
        #endif
        // const byte f[]={0x7B, 0x05, 0x03, 0x01, 0x01, 0x05, 0x7D}; Serial.write(f, 7);
        OEMBuffer[out++] = OEM_CMD_UPDATE;      // Command
        OEMBuffer[out++] = 0x04 ;      // Length

        switch(type){
            case 0x01:      // BOOL
            {
                 /* Node Number */
                if(dpid == DPID_FAN_SWITCH) OEMBuffer[out++] = 0x01 ;
                else OEMBuffer[out++] = dpid+1;
                byte value = tuyaData[idx];

                if(value)
                    OEMBuffer[out++] = OEM_SWITCH_ON;   // ON
                else
                    OEMBuffer[out++] = OEM_SWITCH_OFF;   // OFF
                
                if(dpid == DPID_FAN_SWITCH) OEMBuffer[out++] = (byte) (currentStatus.fan_speed * 25) ;
                else OEMBuffer[out++] = 0x00;

                break;
            }

            case 0x02:      // VALUE
            {
                dpid = 0x01 ;
                int value =(int)tuyaData[idx+3];
                OEMBuffer[out++] = dpid;
                OEMBuffer[out++] = OEM_SWITCH_ON;           // ON
                OEMBuffer[out++] = (byte)(value * 25 );    // Dimmer
                break;
            }
        }

        OEMBuffer[out++] = calcChecksum(OEMBuffer, 6);

        OEMBuffer[out++] = OEM_END_BYTE;
        *oemLen = out ;
        
        return OEMBuffer;
    }
    else if(cmd == TUYA_CMD_PRODUCT_INFO){
        
        

        // 2. Extract Version from the Tuya JSON Payload
        // The JSON looks like: ... "v":"2.1.17" ...
        // We look for "v":" and grab the first digit '2'
        byte verMajor = 0x02; // Default if not found
        String payload = "";
        for (int i = 0; i < tuyaLen - 1; i++) payload += (char)tuyaData[i];
        
        String pid = "";
        int pStart = payload.indexOf("\"p\":\"");
        if (pStart != -1) {
            int pEnd = payload.indexOf("\"", pStart + 5);
            pid = payload.substring(pStart + 5, pEnd);
        }

       
        int vIdx = payload.indexOf("\"v\":\"");
        if (vIdx != -1) {
            verMajor = payload.charAt(vIdx + 5) - '0'; // Extracts '2' from "2.1.17"
        }
        
        
        byte numFans = 1;
        byte numSwitches = 4;
        OEMBuffer[out++] = OEM_CMD_ACK;
        OEMBuffer[out++] = 4 + numFans + numSwitches ;
        OEMBuffer[out++] = verMajor;      // Version (0x02)
        OEMBuffer[out++] = numFans;       // No of Fan (0x01)
        OEMBuffer[out++] = numSwitches;   // No of Switch (0x04)

        // Append Fan Load Types first
        for (int i = 0; i < numFans; i++) {
            OEMBuffer[out++] = 0x04;      // Fan Load Type
        }

        // Append Switch Load Types next
        for (int i = 0; i < numSwitches; i++) {
            OEMBuffer[out++] = 0x01;      // Switch Load Type
        }

        // Calculate Checksum (Sum of all bytes before CS)
        byte cs = 0;
        for (int i = 2; i < out; i++) {
            cs += OEMBuffer[i];
        }
        OEMBuffer[out++] = cs;            // Checksum
        OEMBuffer[out++] = OEM_END_BYTE;          // End Code
        *oemLen = out ;
        
        return OEMBuffer;

    }
    else
        return OEMBuffer;
}

void OemToTuya(String *OemData)
{
   
    byte tuyaFrame[16];
    int i = 0;
    tuyaFrame[i++] = 0x55;
    tuyaFrame[i++] = 0xAA;
    tuyaFrame[i++] = 0x00; // Version
    
    //Serial.println(*OemData);
    if (OemData->length() < 2) return;
    int idx = 0;

    // Access the raw bytes from the String
    const byte* oem = (const byte*)OemData->c_str();
    if (oem[idx++] != OEM_START_BYTE || oem[OemData->length() - 1] != OEM_END_BYTE ) return;

    byte cmd = oem[idx++] ;
    idx++;
    if(cmd == OEM_CMD_UPDATE){
        tuyaFrame[i++] = 0x06; // Command: Send
        tuyaFrame[i++] = 0x00; // Length High
        int dataLen1 = 0; 
        byte dpid  = oem[idx++];
        byte type = 0x01;;
        byte tuyaVal[4];
        int dataLen2 = 0;
        if( dpid == 0x01){
            // Fan Logic
            byte speed = oem[idx+1];
            if (speed > 0) {
                idx++;
                dpid = 0x68; // Fan Speed
                type = 0x02; // Value (4 bytes)
                // Reverse scaling: OEM 0-100 to Tuya 1-5
                // Example: 0x32 (50) / 20 = 2.5 -> 2
                speed = oem[idx++] / (0x19) ;
                tuyaVal[0] = 0x00; tuyaVal[1] = 0x00;
                tuyaVal[2] = 0x00; tuyaVal[3] = speed;
                dataLen2 = 4;
                dataLen1 = 4 + dataLen2;
            } else {
                dpid = 0x66; // Fan Power
                tuyaVal[0] = ( oem[idx++] == 0xFF) ? 0x00 : 0x01;
                dataLen2 = 1;
                dataLen1 = 4 + dataLen2;
            }
        }else{
            dpid -= 1 ;
            tuyaVal[0] = ( oem[idx++] == 0xFF) ? 0x00 : 0x01;
            dataLen2 = 1;
            dataLen1 = 4 + dataLen2;
        }
        tuyaFrame[i++] = (byte)dataLen1;
        tuyaFrame[i++] = dpid;
        tuyaFrame[i++] = type;
        tuyaFrame[i++] = 0x00; // Data Len High
        tuyaFrame[i++] = (byte)dataLen2;
        for (int j = 0; j < dataLen2 ; j++) {
            tuyaFrame[i++] = tuyaVal[j];
        }

        
        // Update our struct
        updateDeviceState(dpid, type, tuyaVal, dataLen2);
        #ifdef DEBUG  
                printCurrentStatus("TUYA_MCU");
        #endif
    }
    else if(cmd == OEM_NODE_STATUS){

        if(oem[idx++] == OEM_ALL_NODE_UPDATE){

            int out = 0;
            OEMBuffer[out++] = 0x7B;          // Start Code
            OEMBuffer[out++] = 0x51;          // Frame Identifier
            
            // Length: 2N + 1. For 5 nodes (Fan + 4 Switches), N=5. Length = 11.
            OEMBuffer[out++] = 11;            

            // --- NODE 1: FAN ---
            OEMBuffer[out++] = currentStatus.fan_power ? 0x00 : 0xFF;
            // Convert fan speed (assuming 1-5) to 0-100 range
            OEMBuffer[out++] = (byte)(currentStatus.fan_speed * 25); 

            // --- NODE 2: SWITCH 1 ---
            OEMBuffer[out++] = currentStatus.switch_1 ? 0x00 : 0xFF;
            OEMBuffer[out++] = 0x00; // No Dimming for simple switch

            // --- NODE 3: SWITCH 2 ---
            OEMBuffer[out++] = currentStatus.switch_2 ? 0x00 : 0xFF;
            OEMBuffer[out++] = 0x00;

            // --- NODE 4: SWITCH 3 ---
            OEMBuffer[out++] = currentStatus.switch_3 ? 0x00 : 0xFF;
            OEMBuffer[out++] = 0x00;

            // --- NODE 5: SWITCH 4 ---
            OEMBuffer[out++] = currentStatus.switch_4 ? 0x00 : 0xFF;
            OEMBuffer[out++] = 0x00;

            // 3. Calculate Checksum (Sum of all bytes before CS)
            byte cs = 0;
            for (int i = 0; i < out; i++) {
                cs += OEMBuffer[i];
            }
            OEMBuffer[out++] = cs;            // Checksum
            OEMBuffer[out++] = 0x7D;          // End Code
            // 4. Send the frame back to the OEM controller/Serial
            
            #ifdef DEBUG
                sendFrame(oemFrame, oemLen, "Converted OEM");
            #endif
            
            // Check if data changed or heartbeat (30s) is needed
            bool hasChanged = (out != lastLen || memcmp(OEMBuffer, lastFrame, out) != 0);
            bool forceSend = (millis() - lastHeartbeat > 30000);

            if (hasChanged || forceSend) {
                if (mqttClient.connected()) {
                mqttClient.publish(mqtt_pub_topic, OEMBuffer, out);
                
                // Update state trackers
                memcpy(lastFrame, OEMBuffer, out);
                lastLen = out;
                lastHeartbeat = millis();
                
                #ifdef DEBUG
                    Serial.println(F("[BRIDGE] Data sent to MQTT."));
                #endif
                }
            }
            *OemData = "";
            return;

        }
    }
    else if(cmd == OEM_ALL_NODE_UPDATE){

        int i = 1 ;
        byte state ;
        byte speed ;
        idx++;
        for( i ; i < 6 ;i++)
        {
            byte state = oem[idx++];
            byte speed = oem[idx++];
            byte Frame[] = {0x7B, 0x00, 0x04, byte(i), state, speed , 0x00 ,0x7D };
            byte cs = 0;
            for(int i=0; i<6; i++) cs += Frame[i];
            Frame[6] = cs;
            //pushToMsgQueue(Frame, sizeof(Frame));
        }
        *OemData = "";
        return;
    }
    else if( cmd == OEM_CMD_DEVICE_INFO){

        tuyaFrame[i++] = 0x01;
        tuyaFrame[i++] = 0x00;
        tuyaFrame[i++] = 0x00;
    }


    byte cs = 0;
    for (int k = 0; k < i; k++) {
        cs += tuyaFrame[k];
    }
    tuyaFrame[i++] = cs;

    *OemData = "";
    for (int k = 0; k < i; k++) {
        *OemData += (char)tuyaFrame[k];
    }

}
