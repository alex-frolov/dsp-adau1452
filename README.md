# dsp-adau1452

It uses adau1452 as DSP. Examples of working projects.


1. Make


#Russian

*Примеры проектов на adau1452.*
**Использовались:**
* [RDC3-0027v1, SigmaDSP ADAU1452. Модуль цифровой обработки звука. V1](https://www.chipdip.ru/product/rdc3-0027v1)
* [USB I2S преобразователь 32bit/96kHz, SUPER PRIME chipdip, USB Hi-Res Audio, квадро, STM32F446RC](https://www.chipdip.ru/product0/9000569733)
* [PCM5102A audio DAC, Преобразователь: I2S - Аудио. Разрешение 32 бит, частота дискретизации 384kHz](https://www.chipdip.ru/product/pcm5102a-audio-dac)
* [PCM1808 audio ADC, Преобразователь: Аудио - I2S. Разрешение 24 бит, частота дискретизации 96kHz](https://www.chipdip.ru/product/pcm1808-audio-adc)
* [SigmaLink-USBi, USBi программатор для SigmaStudio](https://www.chipdip.ru/product/sigmalink-usbi)
* Переменные резисторы 10кОм...100кОм

Всё по цене дешевле чем у китайцев брать что-то похожее и без гаранции, что заработает.

##0. В начале проверяем работоспособность платы.

##1. Версия без микроконтроллера с переменными резисторами. Лежит в папке first.
Входы: USB через SUPER PRIME, китайский bluetooth 5.0 модуль подключенный через PCM1808 к ADAU1452, возможно использовать оставшиеся два входа и SPDIF. Селектор на пять каналов это подразумевает.
Выходы: один PCM5102A серео.
Задействовано 5 переменных резисторов подключенных на AUXDAC0-4:
 - селектор 5 входов, 5ый вход тестовый, генерит шум и синусоиду;
 - Регулятор громкости;
 - регулятор баланса;
 - фильтр низких частот (до 100Гц);
 - фильтр высоких частот (от 5кГц).

У первого входа стоят фильтры от постоянки на входе и фильтры высоких частот от 20Гц, чтобы обрезать всё что ниже.
Никакой особой обработки звука, кроссоверов, параметрических эквалезеров и пр.

Были проблемы с землёй и питанием, у ЦАПа и АЦП не подключал 3.3в ножку, питание через +5в.
USBi так же убрал +3.3в подключение к adau.
Питание от SUPER PRIME, подключил +5в туда внешние. От компа через SUPER PRIME так же ок.
Так же при подключении в параллель USBi и SUPER PRIME - земляная петля часто - треск и помехи.

С SUPER PRIME - были танцы с бубном, прошил с v1.4 до v1.5.1 - заработало всё что с 48кГц и ниже.
Что бы запустить на 32 бита и 96кГц надо в проекте во вкладке CLOCK CONTROL опцию "PLL input clock divider" установить в "Вумшву ин 8".

*На слух какая-то особенность с регулировкой баланса, не совсем разобрался.*

![Макетная плата] (https://github.com/alex-frolov/dsp-adau1452/blob/master/first/maket.jpg)

![Основная схема] (https://github.com/alex-frolov/dsp-adau1452/blob/master/first/main_scheme.png)

![Табица сравнения селектора выходов] (https://github.com/alex-frolov/dsp-adau1452/blob/master/first/channel_select_table.png)

![Настройка ФНЧ] (https://github.com/alex-frolov/dsp-adau1452/blob/master/first/bass_settings.png)

![Настройка ФВЧ] (https://github.com/alex-frolov/dsp-adau1452/blob/master/first/treble_settings.png)

![Вкладка настройки CLOCK CONTROL] (https://github.com/alex-frolov/dsp-adau1452/blob/master/first/clock_control_settings.png)

![Вкладка настройки CORE CONTROL] (https://github.com/alex-frolov/dsp-adau1452/blob/master/first/core_control_settings.png)

![Вкладка настройки SERIAL PORT] (https://github.com/alex-frolov/dsp-adau1452/blob/master/first/serial_port_settings.png)
