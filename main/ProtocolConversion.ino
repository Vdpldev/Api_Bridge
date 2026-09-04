

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

byte* TuyaToOem(byte cmd , byte *tuyaData, int tuyaLen,int *oemLen){
    
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

                OEMBuffer[out++] = 0x00;

                break;
            }

            case 0x02:      // VALUE
            {
                dpid = 0x01 ;
                int value =(int)tuyaData[idx+3];

                if(value > 100)
                    value = 100;
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
    else
        return OEMBuffer;

    ;
}

void OemToTuya(String *OemData)
{

}
