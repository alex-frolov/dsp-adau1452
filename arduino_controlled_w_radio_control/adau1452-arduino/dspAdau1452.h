#ifndef MY_DSPADAU1452_H
#define MY_DSPADAU1452_H

// адрес DSP по шине I2C
#define DSP_I2C_ADDRESS_7BIT 0x38
//#define DSP_SUBADDRESS_MUTE 0x005E
#define DSP_SUBADDRESS_MUTE 0x00E7
#define DSP_SUBADDRESS_CHANNEL 0x00E3
#define DSP_SUBADDRESS_SUPER_BASS 0x6020
#define DSP_SUBADDRESS_STEREO_BASE 0x00E4
#define DSP_SUBADDRESS_DYNAMIC_BASS 0x00E5
#define DSP_SUBADDRESS_VOLUME_LEFT 0x002F
#define DSP_SUBADDRESS_VOLUME_RIGHT 0x0032
#define DSP_SUBADDRESS_HIBERNATE 0xF400

const byte DSP_VOLUMES[42][4] = {
  {0x00,0x00,0x08,0x40},
  {0x00,0x00,0x0A,0x63},
  {0x00,0x00,0x0D,0x13},
  {0x00,0x00,0x10,0x76},
  {0x00,0x00,0x14,0xB9},
  {0x00,0x00,0x1A,0x17},
  {0x00,0x00,0x20,0xD9},
  {0x00,0x00,0x29,0x5A},
  {0x00,0x00,0x34,0x0F},
  {0x00,0x00,0x41,0x89},
  {0x00,0x00,0x52,0x81},
  {0x00,0x00,0x67,0xDE},
  {0x00,0x00,0x82,0xC3},
  {0x00,0x00,0xCF,0x3E},
  {0x00,0x01,0x04,0xE7},
  {0x00,0x01,0x9D,0x81},
  {0x00,0x02,0x08,0x92},
  {0x00,0x02,0x8F,0x5C},
  {0x00,0x03,0x39,0x0D},
  {0x00,0x04,0x0E,0xAD},
  {0x00,0x05,0x1B,0x9D},
  {0x00,0x06,0x6E,0x31},
  {0x00,0x08,0x18,0x6E},
  {0x00,0x0A,0x31,0x09},
  {0x00,0x0C,0xD4,0x95},
  {0x00,0x10,0x27,0x0B},
  {0x00,0x14,0x55,0xB6},
  {0x00,0x19,0x99,0x9A},
  {0x00,0x20,0x3A,0x7E},
  {0x00,0x28,0x92,0xC2},
  {0x00,0x33,0x14,0x27},
  {0x00,0x40,0x4D,0xE6},
  {0x00,0x50,0xF4,0x4E},
  {0x00,0x65,0xEA,0x5A},
  {0x00,0x80,0x4D,0xCE},
  {0x00,0xA1,0x86,0x6C},
  {0x00,0xCB,0x59,0x18},
  {0x01,0x00,0x00,0x00},
  {0x01,0x00,0x00,0x00},
  {0x01,0x00,0x00,0x00},
  {0x01,0x00,0x00,0x00},
  {0x01,0x00,0x00,0x00},  
};

#define MAX_CHANNEL_NUMBER 4
const String DSP_CHANNELS[6] = { "BT", "USB", "Line In", "FM", "SPDIF"};

word Param_address;
byte Cmd_Data[4];

/**
 * Отправка команды в DSP.
 * 
 * @use word Param_address - субадрес
 * @use byte Cmd_Data[4]byte Cmd_Data[4] - команда 4ре байта
 */
void  I2C_16_write()
{
  Wire.beginTransmission(DSP_I2C_ADDRESS_7BIT);         //start transmitting to the ADAU1701 
  Wire.write(highByte(Param_address));    //write subaddress high
  Wire.write(lowByte(Param_address));     //write subaddress low
  for (byte x = 0; x < 4; x++) {    //set up a loop to write 4 bytes
      Wire.write(Cmd_Data[x]);      //write 8-bits of data for each loop iteration
  }           //loop 4 times (32bits total)
  Wire.endTransmission();     //send the stop bit
}

/**
 * Отправка команды в DSP.
 * 
 * @use word Param_address - субадрес
 * @use byte Cmd_Data[2]byte Cmd_Data[2] - команда 2 байта первых
 */
void  I2C_16_write2byte()
{
  Wire.beginTransmission(DSP_I2C_ADDRESS_7BIT);         //start transmitting to the ADAU1701 
  Wire.write(highByte(Param_address));    //write subaddress high
  Wire.write(lowByte(Param_address));     //write subaddress low
  for (byte x = 0; x < 2; x++) {    //set up a loop to write 4 bytes
      Wire.write(Cmd_Data[x]);      //write 8-bits of data for each loop iteration
  }           //loop 2 times (16bits total)
  Wire.endTransmission();     //send the stop bit
}

void dspAdau1452Mute()
{
   Param_address = DSP_SUBADDRESS_MUTE;
   Cmd_Data[0] = 0x00;
   Cmd_Data[1] = 0x00;
   Cmd_Data[2] = 0x00;
   Cmd_Data[3] = 0x00;
   I2C_16_write();
}

void dspAdau1452Unmute()
{
   Param_address = DSP_SUBADDRESS_MUTE;
   Cmd_Data[0] = 0x00;
   Cmd_Data[1] = 0x00;
   Cmd_Data[2] = 0x00;
   Cmd_Data[3] = 0x01;
   I2C_16_write();
}

void dspAdau1452SelectChannel(byte channel)
{
  String name0;

   Param_address = DSP_SUBADDRESS_CHANNEL;
   if (channel == 0) {
     Cmd_Data[0] = 0x00;
     Cmd_Data[1] = 0x00;
     Cmd_Data[2] = 0x00;
     Cmd_Data[3] = 0x00;
     name0 = "BT";
   } else if (channel == 1) {
     Cmd_Data[0] = 0x00;
     Cmd_Data[1] = 0x00;
     Cmd_Data[2] = 0x00;
     Cmd_Data[3] = 0x02;
     name0 = "USB";
   } else if (channel == 2) {
     Cmd_Data[0] = 0x00;
     Cmd_Data[1] = 0x00;
     Cmd_Data[2] = 0x00;
     Cmd_Data[3] = 0x04;
     name0 = "Line In";
   } else if (channel == 3) {
     Cmd_Data[0] = 0x00;
     Cmd_Data[1] = 0x00;
     Cmd_Data[2] = 0x00;
     Cmd_Data[3] = 0x06;
     name0 = "FM";
   } else {
     Cmd_Data[0] = 0x00;
     Cmd_Data[1] = 0x00;
     Cmd_Data[2] = 0x00;
     Cmd_Data[3] = 0x08;
     name0 = "SPDIF";
   }

   I2C_16_write();
}

void dspAdau1452SuperBassOn()
{
   Param_address = DSP_SUBADDRESS_SUPER_BASS;
   Cmd_Data[0] = 0x00;
   Cmd_Data[1] = 0x00;
   Cmd_Data[2] = 0x00;
   Cmd_Data[3] = 0x00;
   I2C_16_write();
}

void dspAdau1452SuperBassOff()
{
   Param_address = DSP_SUBADDRESS_SUPER_BASS;
   Cmd_Data[0] = 0x01;
   Cmd_Data[1] = 0x00;
   Cmd_Data[2] = 0x00;
   Cmd_Data[3] = 0x00;
   I2C_16_write();
}


void dspAdau1452StereoBaseOn()
{
   Param_address = DSP_SUBADDRESS_STEREO_BASE;
   Cmd_Data[0] = 0x00;
   Cmd_Data[1] = 0x00;
   Cmd_Data[2] = 0x00;
   Cmd_Data[3] = 0x02;
   I2C_16_write();
}

void dspAdau1452StereoBaseOff()
{
   Param_address = DSP_SUBADDRESS_STEREO_BASE;
   Cmd_Data[0] = 0x00;
   Cmd_Data[1] = 0x00;
   Cmd_Data[2] = 0x00;
   Cmd_Data[3] = 0x00;
   I2C_16_write();
}

void dspAdau1452DynamicBassOn()
{
   Param_address = DSP_SUBADDRESS_DYNAMIC_BASS;
   Cmd_Data[0] = 0x00;
   Cmd_Data[1] = 0x00;
   Cmd_Data[2] = 0x00;
   Cmd_Data[3] = 0x02;
   I2C_16_write();
}

void dspAdau1452DynamicBassOff()
{
   Param_address = DSP_SUBADDRESS_DYNAMIC_BASS;
   Cmd_Data[0] = 0x00;
   Cmd_Data[1] = 0x00;
   Cmd_Data[2] = 0x00;
   Cmd_Data[3] = 0x00;
   I2C_16_write();
}

void dspAdau1452SetVolume(word addr, int volume, int balance)
{
   int v = 0;
   if (volume - balance > 0 && balance >= 0) {
     v = volume - balance;
   } else if (balance <= 0) {
     v = volume;
   }

   Param_address = addr;

//    Serial.println("VOLUMES");
//    Serial.println(volume);
//    Serial.println(balance);
//    Serial.println(v);
//    Serial.println("cmd");
//    Serial.println(DSP_VOLUMES[v][0]);
//    Serial.println(DSP_VOLUMES[v][1]);
//    Serial.println(DSP_VOLUMES[v][2]);
//    Serial.println(DSP_VOLUMES[v][3]);
   
   Cmd_Data[0] = DSP_VOLUMES[v][0];
   Cmd_Data[1] = DSP_VOLUMES[v][1];
   Cmd_Data[2] = DSP_VOLUMES[v][2];
   Cmd_Data[3] = DSP_VOLUMES[v][3];

   I2C_16_write();
}

void dspAdau1452HibernateOn()
{
   Param_address = DSP_SUBADDRESS_HIBERNATE;
   Cmd_Data[0] = 0x00;
   Cmd_Data[1] = 0x01;
   Cmd_Data[2] = 0x00;
   Cmd_Data[3] = 0x00;
   I2C_16_write2byte();
}

void dspAdau1452HibernateOff()
{
   Param_address = DSP_SUBADDRESS_HIBERNATE;
   Cmd_Data[0] = 0x00;
   Cmd_Data[1] = 0x00;
   Cmd_Data[2] = 0x00;
   Cmd_Data[3] = 0x00;
   I2C_16_write2byte();
}


#endif
