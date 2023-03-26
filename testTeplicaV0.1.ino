#include "SPI.h"
#include "SD.h"
#include "DHT.h"
#include <Wire.h>
#include <TroykaMQ.h>

const int timer = 1;
const int light = 10; // значение пина подсветки
const int waterPump = 9; // значение пина поливочного насоса
const int humidifierPump = 8; // значение пина насоса подачи воды в увлажнитель воздуха
const int humidifier = 7; // значение пина увлажнителя воздуха
const int humidifierWaterControl = A3; // контроль уровня воды в резервуаре увлажнителя
const int TankWaterControl = A2; // контроль уровня воды в основном резервуаре воды
const int chipSelect = 4; // значение пина, к которому подключен модуль SD карты(пин CS)
const int waterAlert = 3; //значение пина сигнализации отсутствия воды в основном баке воды
const int sleepMinutes = 1; // время, через которое будет осуществляться работы микроконтроллера
#define soilControl A1 // значение пина контроля влажности почвы
unsigned long lastTime; // переменная, указывающая время последнего обращения к функции millis() в основном цикле
unsigned long humidifierTime; //время обращения в цикле увлажнителя
unsigned long lightTime; //время обращения в цикле подсветки
unsigned long waterTime; //время обращения в цикле полива почвы
int soilHumidity;
#define DHTPIN 2 // номер пина, к которому подключен датчик температуры и влажности
#define DHTTYPE DHT22 // выбор типа датчика темп. ивлажн. 
DHT dht(DHTPIN, DHTTYPE); // создаём объект для работы с датчиком и передаём ему номер пина

#define PIN_MQ2  A0 //имя для пина, к которому подключен датчик газа
MQ2 mq2(PIN_MQ2); // создаём объект для работы с датчиком и передаём ему номер пина


void setup() {
  //Serial.begin(9600);
  mq2.calibrate(); // калибровка датчика газа
  dht.begin(); // запуск DHT22
  
  pinMode(light, OUTPUT); //инициализация пина света
  pinMode(waterPump, OUTPUT); //инициализация пина водяного насоса
  pinMode(humidifierPump, OUTPUT); //инициализация пина насоса увлажнителя
  pinMode(humidifier, OUTPUT); //инициализация пина увлажнителя
  pinMode(SS, OUTPUT); //инициализация пина модуля SD карты
  pinMode(humidifierWaterControl, INPUT); //инициализация пина контроля уровня воды в увлажнителе
  pinMode(TankWaterControl, INPUT); //инициализация пина контроля уровня воды в основном баке
  pinMode(soilControl, INPUT); // инициализация пина определения влажности почвы
}

void loop() {
  digitalWrite(light, HIGH); //включение подсветки, потом добавить циклы дня и ночи надо.
  //запуск команд внутри цикла производится, когда разница между вызовами функции времени millis() составляет определенное значение
  if (millis()- lastTime > (unsigned long) sleepMinutes * timer * 1000){ 
    lastTime = millis();
    
    String dataString = "";
    int h = dht.readHumidity(); //получение значения влажности воздуха
    int t = dht.readTemperature(); //получение значения температуры воздуха
    int Mh = mq2.readMethane(); // получение значения содержания метана в воздухе(растения выделяют метан)
    int Hydr = mq2.readHydrogen(); // получение значения содержания водорода в воздухе
    soilHumidity = map(analogRead(soilControl),0, 1023, 0, 99); // получение значения влажности почвы в значениях от 0 до 99;
    
    dataString = "Humidity: " + String(h) + "% " + " Temperature: " + String(t) + " Methane: " //генерация строки для файла лога SD карты
    + String(mq2.readMethane()) + " ppm " + "Hydrogen: " + String(mq2.readHydrogen()) + "ppm";

    File dataFile = SD.open("datalog.txt", FILE_WRITE);

  // если файл лога SD карты доступен, в него будут записываться данные с датчиков:
    if (dataFile) { 
      dataFile.println(dataString);
      dataFile.close();}

    
    //Serial.println(h);
    //Serial.println(t);
    //Serial.println(Mh);
    //Serial.println(Hydr);

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
      digitalWrite(humidifier, LOW);}


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


}
