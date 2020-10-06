/*   Данный скетч делает следующее: передатчик (TX) отправляет массив
 *   данных, который генерируется согласно показаниям с кнопок и с энкодера с кнопкой
 *   Приёмник получает массив, и действует аналогично кнопкам и энкодеру в системе с DSP
 *   by Frolov Aleksander 2020
*/

#include <SPI.h>          // библиотека для работы с шиной SPI
#include "nRF24L01.h"     // библиотека радиомодуля
#include "RF24.h"         // ещё библиотека радиомодуля

#define ENCODER_CLK_PIN 2 // пин Энкодера CLK
#define ENCODER_DT_PIN 3 // пин Энкодера DT
#define ENCODER_SW_PIN 8 // пин кнопки энкодера SW

#define BTN_MODE 1 // Кнопка выбора режима (кнопка энкодера)
#define BTN_CHANNEL 2 // Кнопка выбора канала
#define BTN_POWER_OFF 3 // Кнопка вкл/выкл

#define CRC 0x61  // Контрольный байт для индентификации данных

#define SOURCE_BUTTON_PIN 4 // пин для емкостного датчика касания - выбор канала
#define OFF_BUTTON_PIN 5 // пин для емкостного датчика касания - выключение/включение


#include "GyverButton.h"
#include "GyverEncoder.h"

//RF24 radio(9, 10); // "создать" модуль на пинах 9 и 10 Для Уно/Nano
RF24 radio(10, 9); // "создать" модуль на пинах 9 и 10 RF-Nano
//RF24 radio(9,53); // для Меги

// инициализация кнопок выбора канала и выкл/выкл
GButton sourceButton(SOURCE_BUTTON_PIN);
GButton offButton(OFF_BUTTON_PIN);

// инициализация энкодера
Encoder enc1(ENCODER_CLK_PIN, ENCODER_DT_PIN, ENCODER_SW_PIN, TYPE1); // надо пробовать или TYPE1 или TYPE2

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; //возможные номера труб

// массив, хранящий передаваемые данные
// [0] - данные по энкодеру, 0 - не менялось, 1 - поворот вправо, 2 - поворот влево .
// [1] - данные по нажатию кнопок: 0 - не нажимались, BTN_MODE, BTN_CHANNEL, BTN_POWER_OFF
// [2] - контрольный байт, принадлежность посылки конкретному экземпляру передатчика-приёмника
byte transmit_data[3];

void setup() {
  Serial.begin(9600); //открываем порт для связи с ПК
  Serial.println(F("Start"));

  transmit_data[2] = CRC;

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

  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);     //размер пакета, в байтах

  radio.openWritingPipe(address[0]);   //мы - труба 0, открываем канал для передачи данных
  radio.setChannel(0x60);  //выбираем канал (в котором нет шумов!)

  radio.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!

  radio.powerUp(); //начать работу
  radio.stopListening();  //не слушаем радиоэфир, мы передатчик

  // задаем прерывания для энкодера
  attachInterrupt(0, isrCLK, CHANGE);    // прерывание на 2 пине! CLK у энка
  attachInterrupt(1, isrDT, CHANGE);    // прерывание на 3 пине! DT у энка
}

void processRadio()
{
  radio.powerUp(); // включить передатчик
  radio.write(&transmit_data, sizeof(transmit_data)); // отправить по радио
  radio.powerDown(); // выключить передатчик
}


void loop() {

  sourceButton.tick();  // обязательная функция отработки. Должна постоянно опрашиваться
  offButton.tick();  // обязательная функция отработки. Должна постоянно опрашиваться
  enc1.tick();

  // кнопка енкодера - выбор опций
  if (enc1.isRelease()) {
    Serial.println(F("pressEncoderButton"));
    sendCommand(0, BTN_MODE);
  }

  if (enc1.isRight()) {
    Serial.println(1);
    sendCommand(1, 0);
  }
  if (enc1.isLeft())  {
    Serial.println(2);
    sendCommand(2, 0);
  }

  // Выбор канала
  if (sourceButton.isRelease()) {
    Serial.println(F("pressChannelButton"));
    sendCommand(0, BTN_CHANNEL);
  }

  // кнопка выключения, сейчас просто выкл звук, потом, видимо перекючение на виртуальный канал, чтобы меньше потребляло
  if (offButton.isRelease()) {
    Serial.println(F("pressOffButton"));
    sendCommand(0, BTN_POWER_OFF);
  }

}

void sendCommand(byte first, byte last)
{
    transmit_data[0] = first;
    transmit_data[1] = last;
    processRadio();
}

void isrCLK() {
  enc1.tick();  // отработка в прерывании
}

void isrDT() {
  enc1.tick();  // отработка в прерывании
}
