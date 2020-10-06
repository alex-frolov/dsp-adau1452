/**
 * Программа работы с DSP Adau1452
 * Меню содержит:
 * - громкость
 * - баланс
 * - супер басс - вкл/выкл
 * - динамический басс - вкл/выкл
 * - расширенное стерео - вкл/выкл
 * Перемещение между пунктами меню - нажатие на энкодер
 * вкл/выкл или установка громкости/баланса - крутим энкодер
 * 
 * Кнопка выбора источника звука: BT, USB. LineIn, FM, SPDIF
 * Кнопка Выкл - вкл/выкл (переход в режим сна)
 * 
 * Через 7 секунд бездействия переходит в меню - громкость.
 * Сохраняет своё состояние.
 * базовая часть для управления DSP, дополнительно подключаем RF24L01 
 * к 9 (CE), 10 (CSN), 11 (MO), 12 (MI), 13 (SCK) выводам Arduino Nano/Uno
 * 
 * by Frolov Aleksander 2020
 */

#include <Arduino.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif

#define I2C_7BITADDR 0x38 // адрес DSP
#define LCD_7BITADDR 0x27 // адрес LCD

#define SOURCE_BUTTON_PIN 4 // пин для емкостного датчика касания - выбор канала
#define OFF_BUTTON_PIN 5 // пин для емкостного датчика касания - выключение/включение

#define ENCODER_CLK_PIN 2 // пин Энкодера CLK
#define ENCODER_DT_PIN 3 // пин Энкодера DT
#define ENCODER_SW_PIN 8 // пин кнопки энкодера SW

#define WIDTH_LCD 16 // кол-во символов в строке у дисплея
#define HEIGHT_LCD 2 // кол-во строк у дисплея
#define VOLUME_MAX_SIZE 40 // максимальное значение громкости
#define CHAR_WIDTH 5

#define SETTING_TYPE_DEFAULT 0 // тип управления по умолчанию
#define SETTING_TYPE_VOLUME 1 // тип управления громкости
#define SETTING_TYPE_BALANCE 2 // тип управления баланса
#define SETTING_TYPE_SUPER_BASS 3 // тип управления - включение супербаса
#define SETTING_TYPE_DYNAMIC_BASS 4 // тип управления - включения динамического баса
#define SETTING_TYPE_STEREO_BASE 5 // тип управления - включение стерео база
#define SETTING_TYPE_EQ_FIR 6 // тип управления включение FIR эквалайзера
#define SETTING_TYPE_MUTE 10 // тип управления выключение системы

#define MENU_VALUE "Volume         " // Громкость
#define MENU_BALANCE "Balanc          " // баланс
#define MENU_SUPER_BASS "Super Bass      " // супер басс
#define MENU_DYNAMIC_BASS "Dynamic Bass    " // динамический басс
#define MENU_STEREO_BASE "Stereo Base     " // расширенное стерео
#define MENU_EQ_FIR "EQ FIR          " // FIR фильтр для коррекции звучания в помещении

#define SETTING_ON "ON " // вкл
#define SETTING_OFF "OFF" // выкл
#define RETURN_TO_VOLUME_TIME 8000 // Задаем время, через которое меню вернется на пункт Громкость в мс
#define CHECK_LIGHT_TIME 5000 // Задаем время, через которое необходимо перевключить светодиоды Выкл/Вкл исходя из текущей освещённсти в мс

#define MENU_COUNT 6 // кол-во типов меню +1

#define LED_PIN_ON 6 // светодиод активного режима работы
#define LED_PIN_OFF 7 // светодиод сонного режима
#define RF_PIN 0 // фоторезистор подключен

#define CRC 0x61  // Контрольный байт для индентификации данных

#include "dspAdau1452.h"
#include "GyverButton.h"
#include "GyverEncoder.h"

RF24 radio(9,10); // "создать" модуль на пинах 9 и 10 Для Уно
//RF24 radio(9,53); // для Меги

byte address[][6] = {"1Node","2Node","3Node","4Node","5Node","6Node"};  //возможные номера труб
// массив, хранящий передаваемые данные
// [0] - данные по энкодеру, 0 - не менялось, 1 - поворот вправо, 2 - поворот влево .
// [1] - данные по нажатию кнопок: 0 - не нажимались, BTN_MODE, BTN_CHANNEL, BTN_POWER_OFF
// [2] - контрольный байт, принадлежность посылки конкретному экземпляру передатчика-приёмника
byte recieved_data[3];

// инициализация дислея
LiquidCrystal_I2C lcd(LCD_7BITADDR, WIDTH_LCD, HEIGHT_LCD);

// спецсимволы для дисплея
const uint8_t note[8]  = {0x2,0x3,0x2,0xe,0x1e,0xc,0x0};
const uint8_t vol5[8] = {
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
};
const uint8_t vol4[8] = {
  0b11110,
  0b11110,
  0b11110,
  0b11110,
  0b11110,
  0b11110,
  0b11110,
  0b11110,
};
const uint8_t vol3[8] = {
  0b11100,
  0b11100,
  0b11100,
  0b11100,
  0b11100,
  0b11100,
  0b11100,
  0b11100,
};
const uint8_t vol2[8] = {
  0b11000,
  0b11000,
  0b11000,
  0b11000,
  0b11000,
  0b11000,
  0b11000,
  0b11000,
};
const uint8_t vol1[8] = {
  0b10000,
  0b10000,
  0b10000,
  0b10000,
  0b10000,
  0b10000,
  0b10000,
  0b10000,
};

const uint8_t bal0[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b11111,
  0b00000,
  0b00000,
  0b00000,
};
const uint8_t bal1[8] = {
  0b10000,
  0b10000,
  0b10000,
  0b11111,
  0b11111,
  0b10000,
  0b10000,
  0b10000,
};
const uint8_t bal2[8] = {
  0b01000,
  0b01000,
  0b01000,
  0b11111,
  0b11111,
  0b01000,
  0b01000,
  0b01000,
};
const uint8_t bal3[8] = {
  0b00100,
  0b00100,
  0b00100,
  0b11111,
  0b11111,
  0b00100,
  0b00100,
  0b00100,
};
const uint8_t bal4[8] = {
  0b00010,
  0b00010,
  0b00010,
  0b11111,
  0b11111,
  0b00010,
  0b00010,
  0b00010,
};
const uint8_t bal5[8] = {
  0b00001,
  0b00001,
  0b00001,
  0b11111,
  0b11111,
  0b00001,
  0b00001,
  0b00001,
};


//начальный адрес в EEPROM для хранения DacRegister
int eeAddress = 0;
uint32_t lastActivityMls = millis(); // последняя активность с меню
uint32_t lastLightCheckMls = millis(); // последняя проверка освещенности

// временные состояния кнопок и энкодера
struct DacRegister {
  unsigned int sourceButton;
  unsigned int offButton;
  unsigned int encoderButton;
  unsigned int encoder;
  int volume;
  int balance;
  byte currentChannel;
  byte currentType;
  byte superBass;
  byte dynamicBass;
  byte StereoBase;
  byte workStatus;
};

DacRegister dacRegister;

// инициализация кнопок выбора канала и выкл/выкл
GButton sourceButton(SOURCE_BUTTON_PIN);
GButton offButton(OFF_BUTTON_PIN);

// инициализация энкодера
Encoder enc1(ENCODER_CLK_PIN, ENCODER_DT_PIN, ENCODER_SW_PIN, TYPE2);

/**
 * Отображение строки на дисплее по координатам x,y
 * 
 * @param String str - строка сообщения
 * @param int x - координата Х на дисплее
 * @param int y - координата Y на дисплее
 */
void showMessge(String str, int x, int y) {
   lcd.setCursor(0, y);
   lcd.print("                ");
   lcd.setCursor(x,y);
   lcd.print(str);
}

/**
 * Установка надобра спец символов для отображения громкости.
 */
void initValueChars()
{
  lcd.createChar(0, note);
  lcd.createChar(1, vol1);
  lcd.createChar(2, vol2);
  lcd.createChar(3, vol3);
  lcd.createChar(4, vol4);
  lcd.createChar(5, vol5);  
}

/**
 * Установка надобра спец символов для отображения баланса.
 */
void initBalanceChars()
{
  lcd.createChar(0, bal0);
  lcd.createChar(1, bal1);
  lcd.createChar(2, bal2);
  lcd.createChar(3, bal3);
  lcd.createChar(4, bal4);
  lcd.createChar(5, bal5);  
}

void setup()
{
  Serial.begin(9600);

  // Если в eeprom нет нужных данных, задаем первоначальные значения регистра
  if (EEPROM.length() == 0) {
    dacRegister.sourceButton = 1;
    dacRegister.offButton = 0;
    dacRegister.encoderButton = 1;
    dacRegister.encoder = 1;
    dacRegister.volume = 0;
    dacRegister.balance = 0;
    dacRegister.superBass = 0;
    dacRegister.dynamicBass = 0;
    dacRegister.StereoBase = 0;
    dacRegister.currentType = SETTING_TYPE_VOLUME;
    dacRegister.currentChannel = 0;
    dacRegister.workStatus = 0;    
  } else {
    // загружаем регистры из eeprom
    loadRegisters();
    dacRegister.currentType = SETTING_TYPE_VOLUME;
  }

  // задаем пины у выкл/выкл светодиодов
  pinMode(LED_PIN_ON, OUTPUT);
  pinMode(LED_PIN_OFF, OUTPUT);
  
  // включаем дисплей
  lcd.init();
  lcd.backlight();
  lcd.home();
  lcd.setCursor(0,0);
  lcd.print("FrolTP DAC v.1.0");

  sourceButton.setDebounce(50);        // настройка антидребезга (по умолчанию 80 мс)
  sourceButton.setTimeout(300);        // настройка таймаута на удержание (по умолчанию 500 мс)

  offButton.setDebounce(50);        // настройка антидребезга (по умолчанию 80 мс)
  offButton.setTimeout(300);        // настройка таймаута на удержание (по умолчанию 500 мс)

  // HIGH_PULL - кнопка подключена к GND, пин подтянут к VCC (PIN --- КНОПКА --- GND)
  // LOW_PULL  - кнопка подключена к VCC, пин подтянут к GND
  sourceButton.setType(HIGH_PULL);
  offButton.setType(HIGH_PULL);

  // NORM_OPEN - нормально-разомкнутая кнопка
  // NORM_CLOSE - нормально-замкнутая кнопка
  sourceButton.setDirection(NORM_OPEN);
  offButton.setDirection(NORM_OPEN);

  // задаем прерывания для энкодера
  attachInterrupt(0, isrCLK, CHANGE);    // прерывание на 2 пине! CLK у энка
  attachInterrupt(1, isrDT, CHANGE);    // прерывание на 3 пине! DT у энка

  // на врмя установки всего звук выключаем, чтобы максимальной громкостью не оглушило
  dspAdau1452Mute(); 

  //выбираем канал звука
  dspAdau1452SelectChannel(dacRegister.currentChannel);

  //рисуем текущий канал на дисплее
  showMessge("Bx:"+DSP_CHANNELS[dacRegister.currentChannel], 0, 0);

  // устанавливаем вкл/выкл режимов супер баса, динамического баса и расширения стерео
  initSetting(&dacRegister.superBass, dspAdau1452SuperBassOn, dspAdau1452SuperBassOff);
  initSetting(&dacRegister.dynamicBass, dspAdau1452DynamicBassOn, dspAdau1452DynamicBassOff);
  initSetting(&dacRegister.StereoBase, dspAdau1452StereoBaseOn, dspAdau1452StereoBaseOff);

  // устанавливаем громкость на оба канала из сохраненного регистра
  dspAdau1452SetVolume(DSP_SUBADDRESS_VOLUME_LEFT, dacRegister.volume, dacRegister.balance);
  dspAdau1452SetVolume(DSP_SUBADDRESS_VOLUME_RIGHT, dacRegister.volume, dacRegister.balance * -1);

  // инициализация текущего типа настроек на дисплее
  initType();

  // инициализация режима сна
  iniOffMode();

  // после установки всех настроек включаем звук.
  dspAdau1452Unmute();

  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0,15);     //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);     //размер пакета, в байтах

  radio.openReadingPipe(1,address[0]);      //хотим слушать трубу 0
  radio.setChannel(0x60);  //выбираем канал (в котором нет шумов!)

  radio.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!
  
  radio.powerUp(); //начать работу
  radio.startListening();  //начинаем слушать эфир, мы приёмный модуль
}

byte pipeNo;

void checkRadio() {
  while ( radio.available(&pipeNo)) {  // слушаем эфир со всех труб
    radio.read( &recieved_data, sizeof(recieved_data) );         // чиатем входящий сигнал

    if (recieved_data[2] == CRC) { // если совпадает контрольный байт, то выполняем команду.
      setActivity();
      if (recieved_data[0] == 1) {
        encoderRotate(1, 0x00); // если был поворот
      } else if (recieved_data[0] == 2) {
        encoderRotate(-1, 0x00); // если был поворот
      } else if (recieved_data[1] == 1) {
        pressEncoderButton();
      } else if (recieved_data[1] == 2) {
        pressChannelButton();
      } else if (recieved_data[1] == 3) {
        pressOffButton();
      }
    }
  }
}

/**
 * Показывем свечение светодиодов состояния вкл и выкл.
 * Вкл - синий светодиод.
 * Выкл - красный светодиод.
 * Если dacRegister.offButton == 1 - то у нас режим сна, включаем LED_PIN_OFF и выключаем LED_PIN_ON и наоборот.
 * 
 * @use dacRegister
 * @use LED_PIN_ON
 * @use LED_PIN_OFF
 * @use RF_PINRF_PIN
 */
void ledShow()
{
  int photocellReading = analogRead(RF_PIN);
  int LedBrightness;

  // светодиод горит ярче, если уровень освещенности на датчике уменьшается
  // это значит, что мы должны инвертировать считываемые значения от 0-1023 к 1023-0
  //photocellReading = 1023 - photocellReading;

  if (dacRegister.offButton == 1) {
    //теперь мы должны преобразовать диапазон 0-1023 в 0-255, так как именно такой диапазон использует analogWrite
    // красный светодиод менеее яркий ему задаем 255 максимум
    int LedBrightness = map(photocellReading, 0, 1023, 0, 255);
    analogWrite(LED_PIN_ON, 0);
    analogWrite(LED_PIN_OFF, LedBrightness);
  } else {
    //теперь мы должны преобразовать диапазон 0-1023 в 0-255, так как именно такой диапазон использует analogWrite
    // для синего светодиода максимум 105
    int LedBrightness = map(photocellReading, 0, 1023, 0, 105);
    analogWrite(LED_PIN_ON, LedBrightness);
    analogWrite(LED_PIN_OFF, 0);
  }
}

void isrCLK() {
  enc1.tick();  // отработка в прерывании
}

void isrDT() {
  enc1.tick();  // отработка в прерывании
}

void loop()
{
  sourceButton.tick();  // обязательная функция отработки. Должна постоянно опрашиваться
  offButton.tick();  // обязательная функция отработки. Должна постоянно опрашиваться
  enc1.tick();

  checkRadio();

  // Выбор канала
  if (sourceButton.isRelease()) {
    setActivity();
    pressChannelButton();    
  }

  // кнопка выключения, сейчас просто выкл звук, потом, видимо перекючение на виртуальный канал, чтобы меньше потребляло
  if (offButton.isRelease()) {
    setActivity();
    pressOffButton();
  }

  // кнопка енкодера - выбор опций
  if (enc1.isRelease()) {
    setActivity();
    pressEncoderButton();
  }

  if (enc1.isRight()) {
    setActivity();
    encoderRotate(1, 0x00); // если был поворот
  }
  if (enc1.isLeft())  {
    setActivity();
    encoderRotate(-1, 0x00);
  }


  // Если прошло RETURN_TO_VOLUME_TIME RETURN_TO_VOLUME_TIME мс с последней активности в меню, то 
  // переходим на меню - Громкость.
  if (lastActivityMls + RETURN_TO_VOLUME_TIME < millis()) {
    if (dacRegister.currentType != SETTING_TYPE_VOLUME) {
      dacRegister.currentType = SETTING_TYPE_VOLUME;
      initVolumeLine();
    }
    setActivity();
  }

  // Если прошло CHECK_LIGHT_TIME мс с последней провеки освещенности, то 
  // перевключаем светодиоды Вкл/Выкл с учетом текущей освещённости
  if (lastLightCheckMls + CHECK_LIGHT_TIME < millis()) {
    lastLightCheckMls = millis();
    ledShow();
  }  
}

/**
 * обарабтываем нажатие кнопки смены канала
 */
void pressChannelButton()
{
    if (dacRegister.currentChannel == MAX_CHANNEL_NUMBER) {
      dacRegister.currentChannel = 0;
    } else {
      dacRegister.currentChannel++;
    }

    dspAdau1452SelectChannel(dacRegister.currentChannel);
    showMessge("Bx:"+DSP_CHANNELS[dacRegister.currentChannel], 0, 0);  
}

void initSetting(byte* setting, void (*FnOn)(), void (*FnOff)() )
{
  if (*setting == 0) {
    FnOff();
  } else {
    FnOn();
  }
}

void changeSetting(byte* setting, void (*FnOn)(), void (*FnOff)() )
{
  lcd.setCursor(13, 1);
  if (*setting == 1) {
    *setting = 0;
    FnOff();
    lcd.print(SETTING_OFF);
  } else {
    *setting = 1;
    FnOn();
    lcd.print(SETTING_ON);
  }
  saveRegisters();
}

void encoderRotate(int direct, byte type)
{
   if (dacRegister.currentType == SETTING_TYPE_BALANCE) {
    showBalanceLine(direct);
   } else if (dacRegister.currentType == SETTING_TYPE_SUPER_BASS) {
     changeSetting(&dacRegister.superBass, dspAdau1452SuperBassOn, dspAdau1452SuperBassOff);
   } else if (dacRegister.currentType == SETTING_TYPE_DYNAMIC_BASS) {
     changeSetting(&dacRegister.dynamicBass, dspAdau1452DynamicBassOn, dspAdau1452DynamicBassOff);
   } else if (dacRegister.currentType == SETTING_TYPE_STEREO_BASE) {
     changeSetting(&dacRegister.StereoBase, dspAdau1452StereoBaseOn, dspAdau1452StereoBaseOff);
   } else if (dacRegister.currentType == SETTING_TYPE_VOLUME) {
    showVolumeLine(direct);    
   }
}

/**
 * Инициализация режима сна при включении.
 * Если dacRegister.offButton dacRegister.offButton равен 1 - то у нас режим сна.
 * 
 * @use dacRegister
 * @use lcd
 */
void iniOffMode()
{
    if (dacRegister.offButton == 1) {
      lcd.noDisplay(); // выключаем дисплей
      lcd.noBacklight();
      dspAdau1452HibernateOn(); // отправляем команду на сон DSP
    } else {
      lcd.backlight();
      lcd.display();
      dspAdau1452HibernateOff(); // отправляем команду DSP на выход из сна
    }
    ledShow(); // показываем свечение светодиодов вкл/выкл
}

/**
 * триггер обработчик кнопки вкл/выкл
 * - Отправка DSP в сон, из сна
 * - вкл/выкл дисплей
 * - включение красного светодиода ВЫКЛ и отключение синего светодиода Вкл и на оборот
 * @use dacRegister
 * @use lcd
 */
void pressOffButton()
{
    if (dacRegister.offButton == 0) {
      lcd.noDisplay(); // гасим дисплей
      lcd.noBacklight();
      dacRegister.offButton = 1; // статус off регистра - активно
      dacRegister.currentType = SETTING_TYPE_MUTE; // установка режима ВЫКЛ
      dspAdau1452HibernateOn(); // отправляем команду на сон DSP
    } else {
      lcd.backlight(); // включаем дисплей
      lcd.display();
      dacRegister.offButton = 0; // статус off регистра - не активно
      initVolumeLine(); // показ строки с громкостью
      dacRegister.currentType = SETTING_TYPE_VOLUME; // установка режима Громкость
      dspAdau1452HibernateOff(); // отправляем команду DSP на выход из сна
    }

    ledShow(); // показываем свечение светодиодов вкл/выкл
    saveRegisters(); // сохраняем состояние в eeprom
}

void pressEncoderButton()
{
  dacRegister.currentType +=1;
  initType();
}

void initType()
{
   if (dacRegister.currentType == MENU_COUNT || dacRegister.currentType == SETTING_TYPE_VOLUME) {
    dacRegister.currentType = SETTING_TYPE_VOLUME;
    initVolumeLine();
   } else if (dacRegister.currentType == SETTING_TYPE_BALANCE) {
    initBalanceLine();
   } else if (dacRegister.currentType == SETTING_TYPE_SUPER_BASS) {
    initSuperBassLine();
   } else if (dacRegister.currentType == SETTING_TYPE_DYNAMIC_BASS) {
    initDynamicBassLine();
   } else if (dacRegister.currentType == SETTING_TYPE_STEREO_BASE) {
    initStereoBaseLine();
   }
}

void initBalanceLine()
{ 
  initBalanceChars();
  lcd.setCursor(0, 1);
  lcd.print(MENU_BALANCE);

  for (byte i = 0; i<9; i++)
  {
    lcd.setCursor(i+7, 1);
    lcd.printByte(0);
  }

  int cnt = 0;
  int mod = 0;

  if (dacRegister.balance > 0) {
    cnt = (dacRegister.balance + 2) / CHAR_WIDTH;
    mod = (dacRegister.balance + 2) % CHAR_WIDTH;
    if (cnt==0) mod + 2;
  } else if (dacRegister.balance < 0) {
    cnt = (dacRegister.balance - 2) / CHAR_WIDTH;
    mod = 4+(dacRegister.balance - 2) % CHAR_WIDTH;
    //if (cnt==0) mod += 4;
  } else if (dacRegister.balance == 0) {
    cnt=0;
    mod = 2;
  }

  lcd.setCursor(11+cnt, 1);

  if (mod == 0) {
    lcd.printByte(1);
  } else if(mod == 1) {
    lcd.printByte(2);
  } else if(mod == 2) {
    lcd.printByte(3);
  } else if(mod == 3) {
    lcd.printByte(4);    
  } else if(mod == 4) {
    lcd.printByte(5);    
  }
}

void initSuperBassLine()
{ 
  initOnOffLine(MENU_SUPER_BASS, &dacRegister.superBass);
}

void initDynamicBassLine()
{ 
  initOnOffLine(MENU_DYNAMIC_BASS, &dacRegister.dynamicBass);
}

void initStereoBaseLine()
{ 
  initOnOffLine(MENU_STEREO_BASE, &dacRegister.StereoBase);
}

void initOnOffLine(String menu, byte* setting)
{ 
  lcd.setCursor(0, 1);
  lcd.print(menu);
  lcd.setCursor(13, 1);
  if (*setting == 1) {
    lcd.print(SETTING_ON);
  } else {
    lcd.print(SETTING_OFF);
  }
}

void initVolumeLine()
{ 
  initValueChars();
  lcd.setCursor(0, 1);
  lcd.printByte(0);
  lcd.print(MENU_VALUE);
  lcd.setCursor(1, 1);

  unsigned int cnt = dacRegister.volume / CHAR_WIDTH;

  if (cnt > 0)
    for (byte i = 0; i<cnt; i++)
    {
      lcd.setCursor(i+8, 1);
      lcd.printByte(5);
    }

  lcd.setCursor(cnt+8, 1);
  cnt = dacRegister.volume % CHAR_WIDTH;

  if (cnt == 1) {
    lcd.printByte(1);
  } else if(cnt == 2) {
    lcd.printByte(2);
  } else if(cnt == 3) {
    lcd.printByte(3);
  } else if(cnt == 4) {
    lcd.printByte(4);    
  }  
}

void showOneElement(byte pos, byte mod)
{
  lcd.setCursor(pos, 1);  
  if (mod == 1) {
    lcd.printByte(1);
  } else if(mod == 2) {
    lcd.printByte(2);
  } else if(mod == 3) {
    lcd.printByte(3);
  } else if(mod == 4) {
    lcd.printByte(4);
  } else if(mod == 5) {
    lcd.printByte(5);
  } else if(mod == 0) {
    lcd.print(" ");    
  }
}

void showBalanceLine(int inc)
{
  if ((dacRegister.balance == -22 && inc == -1) || (dacRegister.balance == 22 && inc == 1)) {
    return;
  }

  int cntOld = 0;
  int cnt = 0;
  int mod = 0;

  if (dacRegister.balance > 0) {
    cntOld = (dacRegister.balance + 2) / CHAR_WIDTH;
  } else if (dacRegister.balance < 0) {
    cntOld = (dacRegister.balance - 2) / CHAR_WIDTH;
  }

  if (dacRegister.balance + inc > 0) {
    cnt = (dacRegister.balance + inc + 2) / CHAR_WIDTH;
    mod = (dacRegister.balance + inc + 2) % CHAR_WIDTH;
    if (cnt==0) mod + 2;
  } else if (dacRegister.balance + inc < 0) {
    cnt = (dacRegister.balance + inc - 2) / CHAR_WIDTH;
    mod = 4+(dacRegister.balance + inc - 2) % CHAR_WIDTH;
    //if (cnt==0) mod += 4;
  } else if (dacRegister.balance + inc == 0) {
    cnt=0;
    mod = 2;
  }

  if (cntOld != cnt) {
    lcd.setCursor(11+cntOld, 1);
    lcd.printByte(0);
  }

  lcd.setCursor(11+cnt, 1);

  if (mod == 0) {
    lcd.printByte(1);
  } else if(mod == 1) {
    lcd.printByte(2);
  } else if(mod == 2) {
    lcd.printByte(3);
  } else if(mod == 3) {
    lcd.printByte(4);    
  } else if(mod == 4) {
    lcd.printByte(5);    
  }

  dacRegister.balance += inc;
  saveRegisters();
  dspAdau1452SetVolume(DSP_SUBADDRESS_VOLUME_LEFT, dacRegister.volume, dacRegister.balance);
  dspAdau1452SetVolume(DSP_SUBADDRESS_VOLUME_RIGHT, dacRegister.volume, dacRegister.balance * -1);

}


void showVolumeLine(int inc)
{
  if (dacRegister.currentType != SETTING_TYPE_VOLUME) {
    dacRegister.currentType = SETTING_TYPE_VOLUME;
    initVolumeLine();
  }

  if ((dacRegister.volume == 0 && inc == -1) || (dacRegister.volume == VOLUME_MAX_SIZE && inc == 1)) {
    return;
  }

  unsigned int pos = dacRegister.volume / CHAR_WIDTH;
  unsigned int mod = dacRegister.volume % CHAR_WIDTH;

  if (mod > 0 && inc == -1) {
    mod += inc;
    showOneElement(pos+8, mod);
  } else if (mod == 0 && inc == -1) {
    showOneElement(pos+7, 4);
  } else if (mod == 0 && inc == 1) {
    showOneElement(pos+8, 1);
  } else if (inc == 1) {
    mod += inc;
    showOneElement(pos+8, mod);
  }

  dacRegister.volume += inc;
  saveRegisters();
  dspAdau1452SetVolume(DSP_SUBADDRESS_VOLUME_LEFT, dacRegister.volume, dacRegister.balance);
  dspAdau1452SetVolume(DSP_SUBADDRESS_VOLUME_RIGHT, dacRegister.volume, dacRegister.balance * -1);  
}

/**
 * Сохраняем регистр состояния в EEPROM
 */
void saveRegisters()
{
    EEPROM.put(eeAddress, dacRegister);
}

/**
 * Читаем регистр состояния из EEPROMEEPROM
 */
void loadRegisters()
{
  EEPROM.get(eeAddress, dacRegister);
}

/**
 * Установка текущей активности
 */
void setActivity()
{
  lastActivityMls = millis();
}
