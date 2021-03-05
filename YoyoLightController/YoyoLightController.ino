void scanPorts()
{
    for (uint8_t i = 0; i < sizeof(portArray); i++)
    {
        for (uint8_t j = 0; j < sizeof(portArray); j++)
        {
            if (i != j)
            {
                Serial.print("Scanning (SDA : SCL) - " + portMap[i] + " : " + portMap[j] + " - ");
                Wire.begin(portArray[i], portArray[j]);
                check_if_exist_I2C();
            }
        }
    }
}

//This code is a modified version of the code posted on the Arduino forum and other places
void check_if_exist_I2C()
{
    byte error, address;
    int nDevices;
    nDevices = 0;

    for (address = 1; address < 127; address++)
    {
        // The i2c_scanner uses the return value of
        // the Write.endTransmisstion to see if
        // a device did acknowledge to the address.

        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0)
        {
            Serial.print("I2C device found at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.print(address, HEX);
            Serial.println(" !");
            nDevices++;
        }
        else if (error == 4)
        {
            Serial.print("Unknow error at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.println(address, HEX);
        }
    } //for loop
    if (nDevices == 0)
        Serial.println("No I2C devices found");
    else
        Serial.println("**********************************\n");
}