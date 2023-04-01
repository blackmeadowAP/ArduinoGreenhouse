#include "SPI.h"
#include "SD.h"
#include "DHT.h"
#include <Wire.h>


#define light 10 // значение пина подсветки
#define waterPump 9 // значение пина поливочного насоса
#define humidifierPump  8 // значение пина насоса подачи воды в увлажнитель воздуха
#define humidifier  7 // значение пина увлажнителя воздуха
#define ventilation  6 // значение пина кулера вентиляции теплицы

#define chipSelect  4 // значение пина, к которому подключен модуль SD карты(пин CS)
#define waterAlert  3 //значение пина сигнализации отсутствия воды в основном баке воды

#define DHTPIN 2 // номер пина, к которому подключен датчик температуры и влажности
#define DHTTYPE DHT22 // выбор типа датчика темп. ивлажн. 
DHT dht(DHTPIN, DHTTYPE); // создаём объект для работы с датчиком и передаём ему номер пина

#define soilControl A0 // значение пина контроля влажности почвы
#define TankWaterControl  A1 // контроль уровня воды в основном резервуаре воды
#define humidifierWaterControl  A2 // контроль уровня воды в резервуаре увлажнителя

#define PERIOD 6   // период работы в секундах ()
#define WORK 2

const int sleepMinutes = 1; // время, через которое будет осуществляться включение цикла микроконтроллера
const int timer = 3;
uint32_t mainTimer, myTimer;
const int ventilationTime = 20;
boolean state = false;


unsigned long lastTime; // переменная, указывающая время последнего обращения к функции millis() в основном цикле
unsigned long humidifierTime; //время обращения в цикле увлажнителя
unsigned long lightTime; //время обращения в цикле подсветки
unsigned long waterTime; //время обращения в цикле полива почвы
unsigned long currentVentilationTime; //время обращения в цикле обдува теплицы
int soilHumidity;

void setup() {
  Serial.begin(9600);

  dht.begin(); // запуск DHT22
  
  pinMode(light, OUTPUT); //инициализация пина света
  pinMode(waterPump, OUTPUT); //инициализация пина водяного насоса
  pinMode(humidifierPump, OUTPUT); //инициализация пина насоса увлажнителя
  pinMode(humidifier, OUTPUT); //инициализация пина увлажнителя
  pinMode(SS, OUTPUT); //инициализация пина модуля SD карты
  pinMode(ventilation, OUTPUT); //инициализация пина модуля вентиляции
  pinMode(humidifierWaterControl, INPUT); //инициализация пина контроля уровня воды в увлажнителе
  pinMode(TankWaterControl, INPUT); //инициализация пина контроля уровня воды в основном баке
  pinMode(soilControl, INPUT); // инициализация пина определения влажности почвы
  
}

void loop() {

    String dataString = "";
    int h = dht.readHumidity(); //получение значения влажности воздуха
    int t = dht.readTemperature(); //получение значения температуры воздуха
    soilHumidity = map(analogRead(soilControl),1023, 0, 0, 99); // получение значения влажности почвы в значениях от 0 до 99;
    
    dataString = "Humidity: " + String(h) + "% " + " Temperature: " + String(t) + " Soil Humidity: " + String(soilHumidity);
    Serial.println(dataString);
    File dataFile = SD.open("datalog.txt", FILE_WRITE);

  // если файл лога SD карты доступен, в него будут записываться данные с датчиков:
    if (dataFile) { 
      dataFile.println(dataString);
      dataFile.close();}

    
    //Serial.println(h);
    //Serial.println(t);

    if (map(analogRead(humidifierWaterControl), 1023, 0, 0, 99) <= 10) { //если воды в баке с увлажнителем мало, она будет доливаться автоматически из основного бака
      digitalWrite(humidifierPump, HIGH);}

    else {
      digitalWrite(humidifierPump, LOW);}
      
    h = dht.readHumidity();
    
    if (h < 50 ) { // если влажность воздуха низкая, будет включаться увлажнитель. влажность можно повысить только в ограниченном пространстве.
      digitalWrite(humidifier, HIGH);
      if (millis()- humidifierTime > timer*1000){ // проверка влажности каждые timer секунд
        humidifierTime = millis();
        h = dht.readHumidity();}}

    else {
      digitalWrite(humidifier, LOW);
      digitalWrite(ventilation, HIGH);
      if (millis()- humidifierTime > timer*1000){ // проверка влажности каждые timer секунд
        humidifierTime = millis();
        h = dht.readHumidity();}}


    if (map(analogRead(TankWaterControl), 1023, 0, 0, 99) <= 10) { //если воды в основном баке мало, сигнализация активируется и будет пищать пока не нальют воды
      tone(waterAlert, 300, 1000);}
    
    else {
      noTone(waterAlert);}
      
    soilHumidity = map(analogRead(soilControl),1023, 0, 0, 99);
    
    if (soilHumidity <= 50){ //если почва сухая, включится насос полива.
      digitalWrite(waterPump, HIGH);

        if (millis()- waterTime > 2*1000){ // каждые 2 секунды будет проверяться влажность почвы
          waterTime = millis();
          soilHumidity = map(analogRead(soilControl),1023, 0, 0, 99);}}
    
    else {
          digitalWrite(waterPump, LOW);}    


  }
