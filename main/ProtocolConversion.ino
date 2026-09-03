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

byte* TuyaToOem(byte cmd , byte *tuyaData, int tuyaLen, byte oemLen)
{
    byte dpid;
    byte type;

    int idx = 0;
    int out = 0;

    dpid = tuyaData[idx++];
    type = tuyaData[idx++];

    uint16_t dataLen =
        ((uint16_t)tuyaData[idx] << 8) |
         tuyaData[idx + 1];

    idx += 2;
    if(cmd == CMD_SET_DP_VALUE )
    {
        OEMBuffer[out++] = 0x7B;      // Start
        OEMBuffer[out++] = 0x00 ;      // Command
        OEMBuffer[out++] = 0x04;      // Length

        /* Node Number */
        OEMBuffer[out++] = dpid;

        /* Default values */
        OEMBuffer[out++] = 0xFF;
        OEMBuffer[out++] = 0x00;

        switch(type)
        {
            case 0x01:      // BOOL
            {
                byte value = tuyaData[idx];

                if(value)
                    OEMBuffer[out++] = 0x00;   // ON
                else
                    OEMBuffer[out++] = 0xFF;   // OFF

                OEMBuffer[5] = 0x00;
                break;
            }

            case 0x02:      // VALUE
            {
                uint32_t value =
                    ((uint32_t)tuyaData[idx] << 24) |
                    ((uint32_t)tuyaData[idx+1] << 16) |
                    ((uint32_t)tuyaData[idx+2] << 8) |
                    ((uint32_t)tuyaData[idx+3]);

                if(value > 100)
                    value = 100;

                OEMBuffer[4] = 0x00;           // ON
                OEMBuffer[5] = (byte)value;    // Dimmer
                break;
            }
        }

        OEMBuffer[6] = calcChecksum(OEMBuffer, 6);

        OEMBuffer[7] = 0x7D;
        oemLen = 8 ;

        return OEMBuffer;
    }
    else
        return OEMBuffer;
}

