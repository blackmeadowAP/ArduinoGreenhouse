#include <TimeLib.h>

#include <SPI.h>
#include <SD.h>
#include "DHT.h"
#include <Wire.h>
#include "Source1.h"

const int chipSelect = 4;

#define light 5          // значение пина подсветки
#define waterPump 9      // значение пина поливочного насоса
#define humidifierPump 8 // значение пина насоса подачи воды в увлажнитель воздуха
#define humidifier 7     // значение пина увлажнителя воздуха
// #define ventilation  6 // значение пина кулера вентиляции теплицы

Sd2Card card;
SdVolume volume;
SdFile root;

// #define waterAlert  3 //значение пина сигнализации отсутствия воды в основном баке воды(отключено до приобретения емкостного датчика жидкости)

#define DHTPIN 2          // номер пина, к которому подключен датчик температуры и влажности
#define DHTTYPE DHT22     // выбор типа датчика темп. и влажн.
DHT dht(DHTPIN, DHTTYPE); // создаём объект для работы с датчиком и передаём ему номер пина

#define soilControl A0 // значение пина контроля влажности почвы
// #define TankWaterControl  A1 // контроль уровня воды в основном резервуаре воды (отключено до приобретения емкостного датчика жидкости)
// #define humidifierWaterControl  A2 // контроль уровня воды в резервуаре увлажнителя (отключено до приобретения емкостного датчика жидкости)

// const int ventilationTime = 20;
boolean state = false;

int soilHumidity;

const int dawnHour = 0;    // Час начала светового дня
const int dawnMinute = 1;  // Минута начала светового дня
const int duskHour = 13;   // Час начала световой ночи
const int duskMinute = 59; // Минута начала световой ночи

const int lowAirHumidity = 50;     // нижний порог влажности(сухо)
const int humidifierWorktime = 60; // время работы увлажнителя воздуха в секундах
const int humidifierPumpTime = 4;  // время работы насоса резервуара увлажнителя в секундах
const int soilHumidityLevel = 135; // значение влажности почвы, при которой включится автополив
const int soilPumpTime = 120;      // время работы насоса полива почвы в секундах

unsigned long startTimeHumidifier = 0; // Переменная для хранения времени старта
bool isHumidifierOn = false;           // Переменная для хранения состояния увлажнителя

unsigned long startTimeSoilPump = 0; // Переменная для хранения времени старта
bool isSoilPumpOn = false;           // Переменная для хранения состояния автополива

unsigned long startTimeHumidifierPump = 0; // Переменная для хранения времени старта
bool isHumidifierPumpOn = false;           // Переменная для хранения состояния увлажнителя

// Переменная для хранения текущего времени суток
boolean isDay = true;

void setup()
{
    // Serial.begin(9600);
    SD.begin(chipSelect);
    dht.begin(); // запуск DHT22

    pinMode(light, OUTPUT);          // инициализация пина света
    pinMode(waterPump, OUTPUT);      // инициализация пина водяного насоса
    pinMode(humidifierPump, OUTPUT); // инициализация пина насоса увлажнителя
    pinMode(humidifier, OUTPUT);     // инициализация пина увлажнителя
    pinMode(SS, OUTPUT);             // инициализация пина модуля SD карты
    // pinMode(ventilation, OUTPUT); //инициализация пина модуля вентиляции(убрано до приобретения сервоприводов заслонок под отверстия для вентиляции и кулеров)
    // pinMode(humidifierWaterControl, INPUT); //инициализация пина контроля уровня воды в увлажнителе
    // pinMode(TankWaterControl, INPUT); //инициализация пина контроля уровня воды в основном баке (отключено до приобретения емкостного датчика жидкости)
    pinMode(soilControl, INPUT); // инициализация пина определения влажности почвы
}

void loop()
{

    int currentHour = hour();
    int currentMinute = minute();

    Serial.print("Loop at " + currentHour + ":" + currentMinute);

    int currentMinutes = currentHour * 60 + currentMinute;

    int downMinutes = dawnHour * 60 + dawnMinute;
    int duskMinutes = duskHour * 60 + duskMinute;


    bool isBtw = currentMinute < downMinutes && currenMinute > duskMinutes;
    //((currentHour > dawnHour) || (currentHour == dawnHour && currentMinute >= dawnMinute)) && (currentHour < duskHour || (currentHour == duskHour && currentMinute < duskMinute)
    if (isBtw)
    {
        Serial.print("Inside at " + currentHour + ":" + currentMinute);
        // Световой день
        if (!isDay)
        {
            digitalWrite(light, HIGH); // Включаем освещение
           

            int h = dht.readHumidity();                                 // получение значения влажности воздуха
            int t = dht.readTemperature();                              // получение значения температуры воздуха
            soilHumidity = map(analogRead(soilControl), 0, 255, 0, 99); // получение значения влажности почвы в значениях от 0 до 99;

            String dataString = " Time from mc startup: " + String(currentHour) + " : " + String(currentMinute) + " Humidity: " + String(h) + " % " + " Temperature: " + String(t) + " Soil Humidity: " + String(soilHumidity); //+ "TankWaterControl: " + String(TankWaterControl);
            // Serial.println(dataString);
            SaveToSD(dataString);
            // Serial.println(h);
            // Serial.println(t);

           

            CheckHumidity(h, currentHour, currentMinute);

            //OliverTank(currentHour, currentMinute);

            CheckSoilHumidity();

            isDay = true;
            Serial.print("Day starts at " + currentHour + ":" + currentMinute);
        }
    }
    else
    {
        // Световая ночь
        if (isDay)
        { // функция вентиляции вырезана до момента добавления кулеров
            // digitalWrite(light, LOW);
            // digitalWrite(ventilation, HIGH);
            // delay(20000);
            // digitalWrite(ventilation, LOW);
            isDay = false;
            Serial.print("Day ends at " + currentHour + ":" + currentMinute);
        }
    }

    delay(1000); // Ждем 1 секунду
}

void SaveToSD(String dataString) {
    File dataFile = SD.open("datalog.txt", FILE_WRITE);
    // если файл лога SD карты доступен, в него будут записываться данные с датчиков:
    if (dataFile)
    {
        dataFile.println(dataString);
        dataFile.close();
    }
}

void OliverTank(int& currentHour, int& currentMinute)
{
    if (map(analogRead(TankWaterControl), 0, 255, 0, 99) <= 20) { //если воды в основном баке мало, сигнализация активируется и будет пищать пока не нальют воды
        String waterTankLog = "";

        waterTankLog = "Time from mc startup: " + String(currentHour) + ":" + String(currentMinute) + "Low water level in reservoir:" + String(map(analogRead(TankWaterControl), 0, 255, 0, 99));
        //Serial.println(dataString);
        SaveToSD(waterTankLog);
        tone(waterAlert, 300, 1000);
        delay(10000);
        noTone(waterAlert);
    }
}

void CheckSoilHumidity()
{
    soilHumidity = map(analogRead(soilControl), 0, 255, 0, 99);

    if (soilHumidity >= soilHumidityLevel)
    { // если почва сухая, включится насос полива.

        if (!isSoilPumpOn)
        { // Если увлажнитель выключен, включаем его
            digitalWrite(waterPump, HIGH);
            isSoilPumpOn = true;
            startTimeSoilPump = millis(); // Запоминаем время старта
        }
        else if (millis() - startTimeSoilPump >= soilPumpTime * 1000)
        { // Если прошло достаточно времени, выключаем насос полива
            digitalWrite(waterPump, LOW);
            isSoilPumpOn = false;
        }

        String soilHumidityLog = "Time from mc startup: " + String(currentHour) + ":" + String(currentMinute) + "Soil humidity:" + String(soilHumidity) + " Soil pump activated";
        // Serial.println(dataString);
        SaveToSD(soilHumidityLog);
    }
}

void CheckHumidity(int& h, int& currentHour, int& currentMinute)
{
    h = dht.readHumidity();
    if (h < lowAirHumidity)
    { // если влажность воздуха низкая, будет включаться увлажнитель. влажность можно повысить только в ограниченном пространстве.
        String humidifierLog = "";

        humidifierLog = "Time from mc startup: " + String(currentHour) + ":" + String(currentMinute) + "Air humidity:" + String(h) + "humidifier activated";
        // Serial.println(dataString);
        File dataFile = SD.open("datalog.txt", FILE_WRITE);

        // если файл лога SD карты доступен, в него будут записываться данные с датчиков:
        if (dataFile)
        {
            dataFile.println(humidifierLog);
            dataFile.close();
        }

        if (!isHumidifierOn)
        { // Если увлажнитель выключен, включаем его
            digitalWrite(humidifier, HIGH);
            isHumidifierOn = true;
            startTimeHumidifier = millis(); // Запоминаем время старта
        }
        else if (millis() - startTimeHumidifier >= humidifierWorktime * 1000)
        { // Если прошло достаточно времени, выключаем увлажнитель
            digitalWrite(humidifier, LOW);
            isHumidifierOn = false;
        }

        if (!isHumidifierPumpOn)
        { // Если насос увлажнителя выключен, включаем его
            digitalWrite(humidifierPump, HIGH);
            isHumidifierPumpOn = true;
            startTimeHumidifierPump = millis(); // Запоминаем время старта
        }
        else if (millis() - startTimeHumidifierPump >= humidifierPumpTime * 1000)
        { // Если прошло достаточно времени, выключаем насос увлажнителя
            digitalWrite(humidifierPump, LOW);
            isHumidifierPumpOn = false;
        }
    }
}
