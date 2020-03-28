/**
 * Демонстация отправки команды mute в DPS ADAU1452 при помощи Arduino
 * 
 * Схема:
 * DSP подключен по I2C к Arduino к SDA (A4) и SLC (A5) по адресу 0x38
 * К D2 подключена кнопка, которая включает/отключает Mute
 * К Arduino так же по I2C подключен двухстрочный дисплей по адресу 0x270x27
 * Проект SigmaDSP такй де как N2
 */

// адрес ADAU1452
#define I2C_7BITADDR 0x38

// адрес двух строчного дисплея
#define LCD_7BITADDR 0x27

#define MEMLOC 0x005E

#define ctsPin 2 // пин для емкостного датчика касания

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(LCD_7BITADDR,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// адрес для отправки команды/данных в dsp
word Param_address;

// данные для отправки в DSP - 4е байта
byte Cmd_Data[4];

int bttnRegister1;

void setup()
{
  // ctsPin выставляем на получение данных
    pinMode(ctsPin, INPUT);
    lcd.init(); // initialize the lcd 

    // Print a message to the LCD.
    lcd.backlight();
    lcd.setCursor(4,0);
    lcd.print("ADAU1452");

    Serial.begin(9600);
    Serial.println("adau1452 test:");

    bttnRegister1 = 1;
    Wire.begin();
}

void  I2C_16_write()    //the routine is "void" because it doesn't return anything
{
    Wire.beginTransmission(I2C_7BITADDRI2C_7BITADDR);         //start transmitting to the ADAU1452 
    Wire.write(highByte(Param_address));    //write subaddress high
    Wire.write(lowByte(Param_address));     //write subaddress low
    for (byte x = 0; x < 4; x++) {    //set up a loop to write 4 bytes
        Wire.write(Cmd_Data[x]);      //write 8-bits of data for each loop iteration
    }           //loop 4 times (32bits total)
    Wire.endTransmission();     //send the stop bit
}

void ADAU1452_mute()
 {
     Param_address = MEMLOC; 
     Cmd_Data[0] = 0;   //MSB = 0
     Cmd_Data[1] = 0x00;  //ADC on; DAC off
     Cmd_Data[2] = 0x00;
     Cmd_Data[3] = 0x00;
     I2C_16_write();
}

void ADAU1452_unmute()
 {
     Param_address = MEMLOC;
     Cmd_Data[0] = 0;     //MSB = 0
     Cmd_Data[1] = 0x01;  //ADC on; DAC on
     Cmd_Data[2] = 0x00;
     Cmd_Data[3] = 0x00;
     I2C_16_write();
}

void loop()
{
  int ctsValue = digitalRead(ctsPin);

  if(digitalRead(ctsPin)){
    if (bttnRegister1 == 1) {
      bttnRegister1 = 0;
    } else {
      bttnRegister1 = 1;
    }
  }

  while(digitalRead(ctsPin)){ // Пустой цикл для ожидания, пока пользователь отпустит кнопку
  }

  if (bttnRegister1 == 0) {
    ADAU1452_mute();
    lcd.setCursor(1,1);
    lcd.print("     mute   ");
  } else {
    ADAU1452_unmute();
    lcd.setCursor(1,1);
    lcd.print("    unmute ");
  }
}
