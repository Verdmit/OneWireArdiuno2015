/*
    <Программа контроллера модели системы СКУД на Arduino. Использует buzzer, RGBLED и RFID-reader>
    Copyright (C) <2014>  <Ilyin I.I. Zubov Y.M.>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <SPI.h>	// стандартная в Arduino IDE библиотека для работы с SPI шиной. Шта? https://ru.wikipedia.org/wiki/Serial_Peripheral_Interface
#include <Servo.h>  // стандартная в Arduino IDE библиотека для управления сервомоторчиком
#include <OneWire.h>// Библиотека, позволяющая пользоваться протоколом OneWire, не зная протокола OneWire

#define touchPIN 10

const int RGB_LED_PINUMBERS[] = { 5, 6, 7 }; // Номера пинов-выходов для RGB-светодиода
const int SERVO_PINUMBER = 3;  // Номер пина управления сервомоторчиком
const int BUZZER_PINUMBER = 8; // номер пина управления пищалкой
const int BUTTON_PINUMBER = 2; // номер входа, подключенный к кнопке

const int NEW_KEY_CHECK_DELAY = 500; // задержка между проверками поднесённости карты в милисекундах
const int OPEN_CLOSE_DELAY = 3000; // задержка между открытием и закрытием

OneWire ds(touchPIN); // использующийся пин отправляется инициализатору класса OneWire
byte curKey[8];
const byte tehUID[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x00, 0x0}; // tehUID карты, на которую реагируем положительно

bool buttonState = 0; // переменная для хранения состояния кнопки

enum States { // Состояния контроллера:
    wait, check, open, close
}; States state; // текущее состояние

bool rightCard; // Для запоминания в цикле проверки верности карты, верна ли она
bool white;		// Состояние светодиода(горит ли), пока находимся в состоянии ожидания карты
int pos = 0;	// Положение ротора сервомотора (градусы)

Servo myservo; // Экземпляр Servo для управления сервомотором

void setup() {
    Serial.begin(9600);	// Начать общение с компом по последовательному порту
    SPI.begin();			// Инициализация SPI-шины. 
    myservo.attach(SERVO_PINUMBER);	// Инициализация управляющего сервоприводом объекта с указанием пина для работы
	
	//curKey = new byte[8];

    Serial.println("Scan PICC to see UID and type..."); // разговариваем с портом, ага.

    white = false; // До начала работы светодиод не горит
    state = wait; // Работа начинается с состояния ожидания

    // активируем на выход пины для RGB-лампочки:
    pinMode(RGB_LED_PINUMBERS[0], OUTPUT); // red
    pinMode(RGB_LED_PINUMBERS[1], OUTPUT); // green
    pinMode(RGB_LED_PINUMBERS[2], OUTPUT); // blue

    pinMode(BUZZER_PINUMBER, OUTPUT); // активируем на выход пин для гудка
    pinMode(BUTTON_PINUMBER, INPUT); //активируем пин на вход, ждем кнопку
}


void loop() {
    switch (state){
    case wait:
        Waiting();
        return;
    case check:
        Checking();
        return;
    case open:
        Opening();
        return;
    case close:
        Closing();
        return;
    }
}

void Waiting(){
    delay(NEW_KEY_CHECK_DELAY);

    /// мигаем белым:
if (!white){
        digitalWrite(RGB_LED_PINUMBERS[0], HIGH);   // зажигаем красный
        digitalWrite(RGB_LED_PINUMBERS[1], HIGH);   // зажигаем зелёный
        digitalWrite(RGB_LED_PINUMBERS[2], HIGH);   // зажигаем синий
        white = true;
    }
    else{
        digitalWrite(RGB_LED_PINUMBERS[0], LOW);   // тушим красный
        digitalWrite(RGB_LED_PINUMBERS[1], LOW);   // тушим зелёный
        digitalWrite(RGB_LED_PINUMBERS[2], LOW);   // тушим синий
        white = false;
    }
	

	if(SearchKey())
        state = check;
    
	
	// считываем значения с входа кнопки
    buttonState = digitalRead(BUTTON_PINUMBER);
    // проверяем нажата ли кнопка
    // если нажата, то buttonState будет HIGH:
    if (buttonState == HIGH) {
        state = open;
    }
}

void Checking(){
    // Желтеем:
    digitalWrite(RGB_LED_PINUMBERS[0], HIGH);   // зажигаем красный
    digitalWrite(RGB_LED_PINUMBERS[1], HIGH);   // зажигаем зелёный
    digitalWrite(RGB_LED_PINUMBERS[2], LOW);    // тушим синий
    
    delay(500); // Задержечка, ага.

    rightCard = true; // Забываем о предыдущей проверке
    // Проверяем считанный UID:
    for (int i = 0; i < 8; i++) {
        if (tehUID[i] != curKey[i])
            rightCard = false;
    }

    if (rightCard)
        state = open; // Открываемся, если та самая
    else{
        Serial.println("Unknown CARD.");
        digitalWrite(RGB_LED_PINUMBERS[0], HIGH);   // зажигаем красный
        digitalWrite(RGB_LED_PINUMBERS[1], LOW);   // тушим зелёный
        digitalWrite(RGB_LED_PINUMBERS[2], LOW);   // тушим синий
        digitalWrite(BUZZER_PINUMBER, HIGH);
        //buzz(4, 7000, 2000); // buzz the buzzer on pin 4 at 2500Hz for 500 milliseconds
        delay(200); //  buzzing period
        digitalWrite(BUZZER_PINUMBER, LOW);

        state = wait; // Ждём другую, если не
    }
}

void Opening(){
    Serial.println("OPEN");
    digitalWrite(RGB_LED_PINUMBERS[0], LOW);
    digitalWrite(RGB_LED_PINUMBERS[1], HIGH); // Зеленеем
    digitalWrite(RGB_LED_PINUMBERS[2], LOW);
    //delay(500); 
    ServoOpen();
    state = close;
}

void Closing(){
    delay(OPEN_CLOSE_DELAY);
    digitalWrite(RGB_LED_PINUMBERS[1], LOW); // Обеззелёниваемся
    ServoClose();
    state = wait;
}

// Две процедурки поворота ротора сервы на 180:
void ServoOpen(){
    for (pos = 0; pos <= 180; pos += 1){ // Крутим с нулеградусного положения на противоположное по одному градусу
        myservo.write(pos);              // Говорим серве двигаться на градус далее
        delay(15);                       // 15мс ждём завершения поворота 
    }
}
void ServoClose(){
    for (pos = 180; pos >= 0; pos -= 1){
        myservo.write(pos);
        delay(15);
    }
}

bool SearchKey(){
	byte data[8];
	
	ds.search(curKey);

	Serial.print("KEY ");
	for(byte i = 0; i < 8; i++) {
		  Serial.print(curKey[i], HEX);
		    if (i != 7) Serial.print(":");
	}Serial.println("");
	
		//ds.read_bytes(curKey, 8);
    if (curKey[0] & curKey[1] & curKey[2] & curKey[3] & curKey[4] & curKey[5] & curKey[6] & curKey[7] == 0xFF){
      Serial.println("...nothing found!"); 
      return false;
	  ds.reset_search();
    }
    return true;
}

/*
    Программа написана для работы, выполненной из любопытства и, по схождению звёзд, для участия
    в конференции БИП 2014.
    Код распространяется под лицензией GPLv3. На здоровье.
    Текст лицензии: http://www.gnu.org/licenses/gpl-3.0.html
*/
