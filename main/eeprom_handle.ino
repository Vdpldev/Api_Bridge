void saveCredentials(String ssid, String password) 
{
    writeEEPROM(0, ssid, 32);
    writeEEPROM(32, password, 32);
    EEPROM.commit();
}

String readEEPROM(int start, int len) 
{
    String data = "";
    for (int i = start; i < start + len; i++) 
    {
        char c = EEPROM.read(i);
        if (c == 0 || c == 255) break;
        data += c;
    }
    return data;
}

void writeEEPROM(int start, String data, int maxLen) 
{
    for (int i = 0; i < maxLen; i++) 
    {
        EEPROM.write(start + i, (i < data.length()) ? data[i] : 0);
    }
}

// Clear EEPROM
void clearEEPROM() 
{
    for (int i = 0; i < EEPROM_START_ADDR + EEPROM_DATA_SIZE; ++i) 
    {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();
}