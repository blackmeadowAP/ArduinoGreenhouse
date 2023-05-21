#include <TimeLib.h>

#include <SPI.h>
#include <SD.h>
#include "DHT.h"
#include <Wire.h>

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
const int dawnMinute = 0;  // Минута начала светового дня
const int duskHour = 0;    // Час начала световой ночи
const int duskMinute = 10; // Минута начала световой ночи

const int lowAirHumidity = 50;     // нижний порог влажности(сухо)
const int humidifierWorkTime = 30; // время работы увлажнителя воздуха в секундах
const int humidifierPumpTime = 4;  // время работы насоса резервуара увлажнителя в секундах
const int soilHumidityLevel = 135; // значение влажности почвы, при которой включится автополив
const int soilPumpTime = 30;       // время работы насоса полива почвы в секундах

unsigned long startTimeHumidifier = 0; // Переменная для хранения времени старта
bool isHumidifierOn = false;           // Переменная для хранения состояния увлажнителя

unsigned long startTimeSoilPump = 0; // Переменная для хранения времени старта
bool isSoilPumpOn = false;           // Переменная для хранения состояния автополива

unsigned long startTimeHumidifierPump = 0; // Переменная для хранения времени старта
bool isHumidifierPumpOn = false;           // Переменная для хранения состояния увлажнителя

// Переменная для хранения текущего времени суток
boolean isDay = false;

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

    // Serial.println("Loop at " + String(currentHour) + ":" + String(currentMinute));

    int currentMinutes = currentHour * 60 + currentMinute;

    int h = dht.readHumidity(); // получение значения влажности воздуха
    int t = dht.readTemperature();

    ThatTurnsMeOn(currentMinute);
    CheckSoilHumidity(currentHour, currentMinute);
    CheckHumidity(h, currentHour, currentMinute);

    // delay(60000);
}

void ThatTurnsMeOn(int currentMinute)
{
    int downMinutes = dawnHour * 60 + dawnMinute;
    int duskMinutes = duskHour * 60 + duskMinute;
    bool isDayCycle = (currentMinute >= downMinutes) && (currentMinute <= duskMinutes);
    bool isChanged = isDayCycle != isDay;
    isDay = isDayCycle;

    if (isChanged)
    {

        if (isDayCycle)
        {

            digitalWrite(light, HIGH); // Включаем освещение
        }
        else
        {
            // Световая ночь

            //{ // функция вентиляции вырезана до момента добавления кулеров
            digitalWrite(light, LOW);
            // digitalWrite(ventilation, HIGH);
            // delay(20000);
            // digitalWrite(ventilation, LOW);
            // isDay = false;
            // Serial.println("Day ends at " + String(currentHour) + ":" + String(currentMinute));
        }
    }
}

void SaveToSD(String dataString)
{
    File dataFile = SD.open("datalog.txt", FILE_WRITE);
    // если файл лога SD карты доступен, в него будут записываться данные с датчиков:
    if (dataFile)
    {
        dataFile.println(dataString);
        dataFile.close();
    }
}

void CheckSoilHumidity(int &currentHour, int &currentMinute)
{
    soilHumidity = map(analogRead(soilControl), 0, 255, 0, 99);

    if (soilHumidity >= soilHumidityLevel)
    {
        digitalWrite(waterPump, HIGH);
        delay(soilPumpTime * 1000);
        digitalWrite(waterPump, LOW);
    }

    else
    {
        digitalWrite(waterPump, LOW);
    }

    String soilHumidityLog = "Time from mc startup: " + String(currentHour) + ":" + String(currentMinute) + "Soil humidity:" + String(soilHumidity) + " Soil pump activated";
    // Serial.println(dataString);
    SaveToSD(soilHumidityLog);
}

void CheckHumidity(int &h, int &currentHour, int &currentMinute)
{
    h = dht.readHumidity();
    if (h < lowAirHumidity)
    {
        // если влажность воздуха низкая, будет включаться увлажнитель. влажность можно повысить только в ограниченном пространстве.
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

        digitalWrite(humidifier, HIGH);
        delay(humidifierWorkTime * 1000);

        digitalWrite(humidifier, LOW);
    }
    else
    {
        digitalWrite(humidifier, LOW);
    }
}