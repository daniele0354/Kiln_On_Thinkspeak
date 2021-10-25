/*
  This sketch sends the temperature value to a ThingSpeak using the MAX31856 to Thingspeak
  channel using the ThingSpeak API (https://www.mathworks.com/help/thingspeak).

  Requirements:

     ESP8266 Wi-Fi Device
     Arduino 1.8.8+ IDE
     Additional Boards URL: http://arduino.esp8266.com/stable/package_esp8266com_index.json
     Library: esp8266 by ESP8266 Community
     Library: ThingSpeak by MathWorks
     Library  Adafruit_MAX31856.h
  ThingSpeak Setup:

     Sign Up for New User Account - https://thingspeak.com/users/sign_up
     Create a new Channel by selecting Channels, My Channels, and then New Channel
     Enable one field
     Enter SECRET_CH_ID in "secrets.h"
     Enter SECRET_WRITE_APIKEY in "secrets.h"

  Setup Wi-Fi:
    Enter SECRET_SSID in "secrets.h"
    Enter SECRET_PASS in "secrets.h"

  Tutorial: http://nothans.com/measure-wi-fi-signal-levels-with-the-esp8266-and-thingspeak

  Created: Feb 1, 2017 by Hans Scharler (http://nothans.com)
*/
void (*Riavvia)(void) = 0;
//#include <Arduino>
#include <Adafruit_MAX31856.h>
#include "ThingSpeak.h"
#include "secrets.h"
#include <ESP8266WiFi.h>
#include <Servo.h>

char ssid[] = SECRET_SSID;  // your network SSID (name)
char pass[] = SECRET_PASS;  // your network password
int keyIndex = 0;           // your network key index number (needed only for WEP)
unsigned long myChannelNumber = SECRET_CH_ID;
const char* myWriteAPIKey = SECRET_WRITE_APIKEY;
char BLOW = LOW;
char STOP = HIGH;
float T_max = 650;
float T_min = 100;
byte int_tx;
byte IoT_on = 15;
byte valv_01 = 2;     //(valv_01,INPUT);    //valv_01 = 2     attivazione valvola in modo manuale
byte auto_man = 14;   //(auto_man, INPUT);  // auto_man = 14  selettore AUTO/MAN
byte iniettore = 13;  //(iniettore, OUTPUT);//iniettore= 13   Uscita avviamneto ventilatore
//servo  = 12     servo movimento valvola
float aperto = 0;
float chiuso = 45;

unsigned long previousMillis = 0;  // will store last time LED was updated
// constants won't change:
const long interval = 1000;  // interval at which to blink (milliseconds)
float DeltaT0;
float DeltaT1;
float
  DeltaT;
char number1;
char number2;
// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31856 maxthermo = Adafruit_MAX31856(16, 5, 4, 0);
// use hardware SPI, just pass in the CS pin
//Adafruit_MAX31856 maxthermo = Adafruit_MAX31856(10);

WiFiClient client;
Servo valvola;



//***********************gestione INTRRUPT *******************

#define ledPin 15  // or define ledPin 5
#define timer0_preload 40161290
#define my_delay 40  // Set interrupt time in secs. Value = 2 x Number of Seconds, so 10-Secs = 10 x 2 = 20
volatile int toggle;

void inline handler(void) {
  toggle = (toggle == 1) ? 0 : 1;
  int_tx = toggle;
  digitalWrite(ledPin, toggle);
  timer0_write(ESP.getCycleCount() + timer0_preload * my_delay);  //
}
void setup() {
  pinMode(valv_01, INPUT);     //valv_01 = 2     attivazione valvola in modo manuale
  pinMode(auto_man, INPUT);    // auto_man = 14  selettore AUTO/MAN
  pinMode(iniettore, OUTPUT);  //iniettore= 13   Uscita avviamneto ventilatore
  pinMode(IoT_on, OUTPUT);     //IoT_on =15      LED  comunicazione attiva
  valvola.attach(12);          //servo  = 12     servo movimento valvola
  valvola.write(0);            //azzeramento e test servo
  Serial.begin(115200);
  delay(100);
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);
  Serial.println("MAX31856 thermocouple test");

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);
    }
    Serial.println("\nConnected.");
  }

  Serial.println("MAX31856 thermocouple test");
  maxthermo.begin();
  maxthermo.setThermocoupleType(MAX31856_TCTYPE_S);

  Serial.print("Thermocouple type: ");
  switch (maxthermo.getThermocoupleType()) {
    case MAX31856_TCTYPE_B: Serial.println("B Type"); break;
    case MAX31856_TCTYPE_E: Serial.println("E Type"); break;
    case MAX31856_TCTYPE_J: Serial.println("J Type"); break;
    case MAX31856_TCTYPE_K: Serial.println("K Type"); break;
    case MAX31856_TCTYPE_N: Serial.println("N Type"); break;
    case MAX31856_TCTYPE_R: Serial.println("R Type"); break;
    case MAX31856_TCTYPE_S: Serial.println("S Type"); break;
    case MAX31856_TCTYPE_T: Serial.println("T Type"); break;
    case MAX31856_VMODE_G8: Serial.println("Voltage x8 Gain mode"); break;
    case MAX31856_VMODE_G32: Serial.println("Voltage x8 Gain mode"); break;
    default: Serial.println("Unknown"); break;
  }
  //****INTERRUPT
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);  // Turn OFF LED to start with
  noInterrupts();
  timer0_isr_init();
  timer0_attachInterrupt(handler);
  timer0_write(ESP.getCycleCount() + timer0_preload * my_delay);
  interrupts();
  DeltaT0 = maxthermo.readThermocoupleTemperature();
}  //END Setup()


//*******************L O O P ******************************
void loop() {
  Serial.print("T   °C   ");
  Serial.println(maxthermo.readThermocoupleTemperature());
  Serial.println();
  //float aperto=0;
  //float chiuso=0;
  // *************** temporizzatore
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last value
    previousMillis = currentMillis;
    /*DeltaT1 = maxthermo.readThermocoupleTemperature();
      DeltaT = DeltaT1 - DeltaT0;
      DeltaT0 = DeltaT1;
      number2 = DeltaT;*/
  }
  if (digitalRead(auto_man) == HIGH)  //condizione in automatico
  {
    Serial.print("automatico  ");
    if (maxthermo.readThermocoupleTemperature() > T_min)  //Se la temperatura del forno è > di Tmin significa che il forno è in processo
    {
      if (maxthermo.readThermocoupleTemperature() < T_max)  //se sono sotto Tmax ne definiscole condizioni
      {
        digitalWrite(iniettore, STOP);
        valvola.write(aperto);
        Serial.print("Valvola aperta");
      } else {
        digitalWrite(iniettore, BLOW);
        valvola.write(chiuso);
        Serial.print("Valvola chiusa");
      };

      if (int_tx == 1) {
        digitalWrite(IoT_on, LOW);
        Serial.println("scrivo canale");
        Serial.println(maxthermo.readThermocoupleTemperature());

        if (WiFi.status() != WL_CONNECTED) {
          Serial.print("Attempting to connect to SSID: ");
          Serial.println(SECRET_SSID);
          while (WiFi.status() != WL_CONNECTED) {
            WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
            Serial.print(".");
            delay(5000);
          }
          Serial.println("\nConnected.");
        }

        // Write value to Field 1 of a ThingSpeak Channel
        char Temp = maxthermo.readThermocoupleTemperature();
        DeltaT1 = Temp;
        DeltaT = DeltaT1 - DeltaT0;
        DeltaT0 = DeltaT1;
        number2 = DeltaT;


        Serial.print(" Temp   ");
        Serial.println(Temp);
        Serial.println();
        ThingSpeak.setField(1, Temp);
        ThingSpeak.setField(2, DeltaT);
        Serial.print("Temp  ->");
        Serial.println(Temp);
        Serial.print("DeltaT  ->");
        Serial.println(DeltaT);
        //int httpCode = ThingSpeak.writeField(myChannelNumber, 1, maxthermo.readThermocoupleTemperature(), myWriteAPIKey);
        int httpCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

        Serial.print(httpCode);  //*************
        client.stop();
        if (httpCode == 200)

        {
          Serial.println("Channel write successful.");
          digitalWrite(IoT_on, HIGH);
        } else {
          Serial.println("Problem writing to channel. HTTP error code " + String(httpCode));
          digitalWrite(IoT_on, LOW);
          Serial.print("Fault to connect Thingspeach");
          Riavvia();
        };
        // Check and print any faults
        uint8_t fault = maxthermo.readFault();
        if (fault) {
          if (fault & MAX31856_FAULT_CJRANGE) Serial.println("Cold Junction Range Fault");
          if (fault & MAX31856_FAULT_TCRANGE) Serial.println("Thermocouple Range Fault");
          if (fault & MAX31856_FAULT_CJHIGH) Serial.println("Cold Junction High Fault");
          if (fault & MAX31856_FAULT_CJLOW) Serial.println("Cold Junction Low Fault");
          if (fault & MAX31856_FAULT_TCHIGH) Serial.println("Thermocouple High Fault");
          if (fault & MAX31856_FAULT_TCLOW) Serial.println("Thermocouple Low Fault");
          if (fault & MAX31856_FAULT_OVUV) Serial.println("Over/Under Voltage Fault");
          if (fault & MAX31856_FAULT_OPEN) Serial.println("Thermocouple Open Fault");
        }
        int_tx = 0;
      };  //END if int_tx

    } else {
      if ((digitalRead(iniettore) == STOP) and (maxthermo.readThermocoupleTemperature() < 30)) {
        digitalWrite(iniettore, BLOW);
        valvola.write(chiuso);
        Serial.print("Valvola chiusa");
      };
    };  //  END cicle :  if (maxthermo.readThermocoupleTemperature()>30)
  } else {
    //Manual  loop
    if (digitalRead(valv_01) == LOW) {
      digitalWrite(iniettore, STOP);
      valvola.write(aperto);
      Serial.print("Valvola aperta manuale");
    } else {
      digitalWrite(iniettore, BLOW);
      valvola.write(chiuso);
      Serial.print("Valvola chiusa manuale");
    };
  };
  delay(100);
}
