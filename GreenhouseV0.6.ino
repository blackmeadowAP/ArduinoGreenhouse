#include <TimeLib.h>

#include <SPI.h>
#include <SD.h>
#include "DHT.h"
#include <Wire.h>

const int chipSelect = 4;

#define light 5 // значение пина подсветки
#define waterPump 9 // значение пина поливочного насоса
#define humidifierPump  8 // значение пина насоса подачи воды в увлажнитель воздуха
#define humidifier  7 // значение пина увлажнителя воздуха
#define ventilation  6 // значение пина кулера вентиляции теплицы

Sd2Card card;
SdVolume volume;
SdFile root;

#define waterAlert  3 //значение пина сигнализации отсутствия воды в основном баке воды

#define DHTPIN 2 // номер пина, к которому подключен датчик температуры и влажности
#define DHTTYPE DHT22 // выбор типа датчика темп. ивлажн. 
DHT dht(DHTPIN, DHTTYPE); // создаём объект для работы с датчиком и передаём ему номер пина

#define soilControl A0 // значение пина контроля влажности почвы
#define TankWaterControl  A1 // контроль уровня воды в основном резервуаре воды
//#define humidifierWaterControl  A2 // контроль уровня воды в резервуаре увлажнителя

const int ventilationTime = 20;
boolean state = false;

int soilHumidity;

const int dawnHour = 0; // Час начала светового дня
const int dawnMinute = 1; // Минута начала светового дня
const int duskHour = 13; // Час начала световой ночи
const int duskMinute = 59; // Минута начала световой ночи

// Переменная для хранения текущего времени суток
boolean isDay = true;

void setup() {
  //Serial.begin(9600);
  SD.begin(chipSelect);
  dht.begin(); // запуск DHT22

  pinMode(light, OUTPUT); //инициализация пина света
  pinMode(waterPump, OUTPUT); //инициализация пина водяного насоса
  pinMode(humidifierPump, OUTPUT); //инициализация пина насоса увлажнителя
  pinMode(humidifier, OUTPUT); //инициализация пина увлажнителя
  pinMode(SS, OUTPUT); //инициализация пина модуля SD карты
  pinMode(ventilation, OUTPUT); //инициализация пина модуля вентиляции
  //pinMode(humidifierWaterControl, INPUT); //инициализация пина контроля уровня воды в увлажнителе
  pinMode(TankWaterControl, INPUT); //инициализация пина контроля уровня воды в основном баке
  pinMode(soilControl, INPUT); // инициализация пина определения влажности почвы

}

void loop() {

  int currentHour = hour();
  int currentMinute = minute();
  //Serial.println("Start");
  //Serial.print("Hour: ");
  //Serial.print(currentHour);
  //Serial.print("Minute: ");
  //Serial.print(currentMinute);
  if ((currentHour >= dawnHour && currentMinute >= dawnMinute) && (currentHour < duskHour || (currentHour == duskHour && currentMinute < duskMinute))) {
    // Световой день
    if (!isDay) {
      digitalWrite(light, HIGH);
      // Включаем освещение


      String dataString = "";
      int h = dht.readHumidity(); //получение значения влажности воздуха
      int t = dht.readTemperature(); //получение значения температуры воздуха
      soilHumidity = map(analogRead(soilControl),0, 255, 0, 99); // получение значения влажности почвы в значениях от 0 до 99;

      dataString = "Time from mc startup: " + String(currentHour)+ ":" + String(currentMinute) + "Humidity: " + String(h) + "% " + " Temperature: " + String(t) + " Soil Humidity: " + String(soilHumidity) + "TankWaterControl: " + String(TankWaterControl);
      //Serial.println(dataString);
      File dataFile = SD.open("datalog.txt", FILE_WRITE);

      // если файл лога SD карты доступен, в него будут записываться данные с датчиков:
      if (dataFile) {
        dataFile.println(dataString);
        dataFile.close();
      }


      //Serial.println(h);
      //Serial.println(t);

      
        

      
      h = dht.readHumidity();

      if (h < 50 ) { // если влажность воздуха низкая, будет включаться увлажнитель. влажность можно повысить только в ограниченном пространстве.
      String humidifierLog = "";
      
      humidifierLog = "Time from mc startup: " + String(currentHour)+ ":" + String(currentMinute) + "Air humidity:" + String(h) + "humidifier activated";
      //Serial.println(dataString);
      File dataFile = SD.open("datalog.txt", FILE_WRITE);

      // если файл лога SD карты доступен, в него будут записываться данные с датчиков:
      if (dataFile) {
        dataFile.println(humidifierLog);
        dataFile.close();
      }
        digitalWrite(humidifier, HIGH);
        delay(60000);
        digitalWrite(humidifier, LOW);
        digitalWrite(humidifierPump, HIGH);
        delay(4000);
        digitalWrite(humidifierPump, LOW);
      
      }


      if (map(analogRead(TankWaterControl), 0, 255, 0, 99) <= 20) { //если воды в основном баке мало, сигнализация активируется и будет пищать пока не нальют воды
      String waterTankLog = "";
      
      waterTankLog = "Time from mc startup: " + String(currentHour)+ ":" + String(currentMinute) + "Low water level in reservoir:" + String(map(analogRead(TankWaterControl), 0, 255, 0, 99)) ;
      //Serial.println(dataString);
      File dataFile = SD.open("datalog.txt", FILE_WRITE);

      // если файл лога SD карты доступен, в него будут записываться данные с датчиков:
      if (dataFile) {
        dataFile.println(waterTankLog);
        dataFile.close();
      }
        tone(waterAlert, 300, 1000);
        delay(10000);
        noTone(waterAlert);
      }



      soilHumidity = map(analogRead(soilControl),0, 255, 0, 99);

      if (soilHumidity >= 120) { //если почва сухая, включится насос полива.
        digitalWrite(waterPump, HIGH);
        delay(10000);
        digitalWrite(waterPump,LOW);
       String soilHumidityLog = "Time from mc startup: " + String(currentHour)+ ":" + String(currentMinute) + "Soil humidity:" + String(soilHumidity) + "Soil pump activated";
      //Serial.println(dataString);
      File dataFile = SD.open("datalog.txt", FILE_WRITE);

      // если файл лога SD карты доступен, в него будут записываться данные с датчиков:
      if (dataFile) {
        dataFile.println(soilHumidityLog);
        dataFile.close();
      }

      }

     

      isDay = true;
    }
  }

  else {
    // Световая ночь
    if (isDay) {
      // Выключаем освещение
      digitalWrite(light, LOW);
      digitalWrite(ventilation, HIGH);
      delay(20000);
      digitalWrite(ventilation, LOW);
      isDay = false;
    }
  }

  delay(1000); // Ждем 1 секунду
}
