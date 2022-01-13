// Libraries
#include <LiquidCrystal_I2C.h>
#include <Thermocouple.h>
#include <MAX6675_Thermocouple.h>
#include <ThingSpeak.h>
#include <Wire.h>
#include <dht.h>
#include "WiFiEsp.h"
#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>

#ifndef HAVE_HWSERIAL3
#include "SoftwareSerial.h"
SoftwareSerial Serial3(6, 7);                             // RX, TX (Software Serial defined)
#endif

dht DHT;

// Pin Definitions
/*
#define TX_PIN_PZEM 22                                    // PZEM-004T Tx Pin
#define RX_PIN_PZEM 23                                    // PZEM-004T Rx Pin
*/

#define trigPin 24                                        // HCSR04 Trig Pin
#define echoPin 25                                        // HCSR04 Echo Pin
  
#define CS_PINW 26                                        // Winding MAX6675 CS Pin
#define SO_PINW 27                                        // Winding MAX6675 SO Pin
#define SCK_PINW 28                                       // Winding MAX6675 SCK Pin

#define CS_PINO 29                                        // Oil MAX6675 CS Pin
#define SO_PINO 30                                        // Oil MAX6675 SO Pin
#define SCK_PINO 31                                       // Oil MAX6675 SCK Pin

#define DHT11_PIN 34                                      // DHT11 Data Pin

// LCDs defined
LiquidCrystal_I2C lcd1(0x21, 16, 2);                      // LCD 1
//LiquidCrystal_I2C lcd2(0x27, 16, 2);                    // LCD 2

Thermocouple* thermocoupleW;                              // Winding Temperature Thermocouple
Thermocouple* thermocoupleO;                              // Oil Temperature Thermocouple

// Network information
const char* ssid = "Mitra_2.4GHz";                        // WiFi Name/SSID
const char* password = "MUNUA2001";                       // WiFi Password

// ThingSpeak information
char thingSpeakAddress[] = "api.thingspeak.com";          // ThingSpeak Address
char* writeAPIKey = "DHNDHNM9Z9G7ZPWV";                   // Write API Key
char* readAPIKey = "9S7OHAFY7AZ9768C";                    // Read API Key
const long channelID = 1274361;                           // Upload Channel ID

//Blynk Information
char auth[] = "Hq3OHNa1u_eubVi6XmX5RIEuAE0NcWRD";         //Blynk Auth Token

const unsigned long postingInterval = 60L * 1000L;        //  Posting Interval/Time period defined

// ThingSpeak DataFields Defined
unsigned int dataFieldOne = 1;                            // Winding Temperature DataField                           
unsigned int dataFieldTwo = 2;                            // Oil Temperature DataField                     
unsigned int dataFieldThree = 3;                          // Ambient Temperature DataField
unsigned int dataFieldFour = 4;                           // Humidity DataField                          
unsigned int dataFieldFive = 5;                           // Distance DataField                          

// Check Variables(Time) for Posting Status defined and set to 0
unsigned long lastConnectionTime = 0;                     
long lastUpdateTime = 0; 

WiFiEspClient client;                                     // WiFi ESP Client defined         
ESP8266 wifi(&Serial3);

// Variables Defined
double duration;                                          // Variable for the duration of sound wave travel
float distance;                                           // Variable for the distance measurement

void setup() {
  
  pinMode(trigPin, OUTPUT);                               // Sets the trigPin as an OUTPUT
  pinMode(echoPin, INPUT);                                // Sets the echoPin as an INPUT
  
  Serial.begin(115200);                                   // Initialize serial for ESP module
  Serial3.begin(115200);                                  // Initialize ESP module
  
  WiFi.init(&Serial3);                                    //WiFi connection initialised through Serial 3 (ESP Module)
  connectWiFi();                                          //Function invoked to connect to specified WiFi
  
  // LCDs initialized
  // LCD 1
  lcd1.begin();
  lcd1.backlight();
  lcd1.clear();
  lcd1.setCursor(0,0);
  /*
  // LCD 2
  lcd2.begin();
  lcd2.backlight();
  lcd2.clear();
  lcd2.setCursor(0,0);
  */
  
  thermocoupleW = new MAX6675_Thermocouple(SCK_PINW, CS_PINW, SO_PINW);             // Defines variable for Winding MAX6675 parameters
  thermocoupleO = new MAX6675_Thermocouple(SCK_PINO, CS_PINO, SO_PINO);             // Defines variable for Oil MAX6675 parameters

  Blynk.begin(auth, wifi, ssid, password);                  //Blynk initiated
   
  delay(10);
  
}


void loop() {

  // Ultrasonic Sensor for Oil Level Monitoring
  digitalWrite(trigPin, LOW);                               // Clears the trigPin condition
  delay(250);
  digitalWrite(trigPin, HIGH);                              // Sets the trigPin HIGH (ACTIVE) for 10 microseconds
  delay(250);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);                        // Reads the echoPin, returns the sound wave travel time in microseconds
  distance = duration * 0.0343 / 2;                         // Calculating the distance : Speed of Sound Wave divided by 2 (Forward and Return)       
  Serial.print("\nDistance: ");
  Serial.print(distance);                                   // Displays the distance on the Serial Monitor
  Serial.println(" cm");

  // DHT11 for Ambient Temperature and Humidity
  int chk = DHT.read11(DHT11_PIN);                          // Reads the DHT11 parameters
  Serial.print("Ambient Temperature = ");
  Serial.println(DHT.temperature);                          // Displays Ambient Temperature in Serial Monitor
  Serial.print("Humidity = ");
  Serial.println(DHT.humidity);                             // Displays Humidity in Serial Monitor
  
  // 2x MAX6675 for Winding Temperature and Oil Temperature
  const double celsiusW = thermocoupleW->readCelsius();     // Reads Winding Temperature
  const double celsiusO = thermocoupleO->readCelsius();     // Reads Oil Temperature
  Serial.print("Temperature of Winding: ");
  Serial.print(celsiusW);                                   // Displays Winding Temperature on Serial Monitor
  Serial.print("\nTemperature of Oil: ");
  Serial.print(celsiusO);                                   // Displays Oil Temperature on Serial Monitor
  Serial.print("\n\n");

  // Checks if last posted data is equal to set Posting Interval
  if (millis() - lastUpdateTime >=  postingInterval) 
  {   
      lastUpdateTime = millis();                            // Sets last update time as current time
      
      // Invokes function to send collected sensor data to ThingSpeak
      write2TSData( channelID , dataFieldOne , celsiusW , dataFieldTwo , celsiusO , dataFieldThree , DHT.temperature , dataFieldFour , DHT.humidity , dataFieldFive , distance );  

      // Writes Data to Blynk
      Blynk.virtualWrite(V5, celsiusW);                     //Sends Winding Temperature through Virtual Pin 5
      Blynk.virtualWrite(V6, celsiusO);                     //Sends Oil Temperature through Virtual Pin 5
      Blynk.virtualWrite(V7, DHT.temperature);              //Sends Ambient Temperature through Virtual Pin 5
  }
    
  // Display on 2x LCD screens
 
  //LCD 1 
  lcd1.clear();
  lcd1.print("W.T:");
  lcd1.print(int(celsiusW));
  lcd1.print("C O.T:");
  lcd1.print(int(celsiusO));
  lcd1.print("C");
  lcd1.setCursor(0,1);
  lcd1.print("V: ?");
  lcd1.print("V I: ?");
  lcd1.print("A");

  //LCD 2
/*lcd2.clear();
  lcd2.print("A.T:");
  lcd2.print(int(DHT.temperature));
  lcd2.print("H:");
  lcd2.print(int(DHT.humidity));
  lcd2.print("% ");
  lcd2.setCursor(0,1);
  lcd2.print("D:");
  lcd2.print(distance);
  lcd2.print("cm"); 
*/

  Blynk.run();

  delay(500);
  
}

// Function for Connection to Wifi
int connectWiFi()
{
    while (WiFi.status() != WL_CONNECTED)                     // Loop until WiFi connected
    {                             
        WiFi.begin( ssid, password );
        delay(2500);
        Serial.println("Connecting to WiFi");
    }
    
    Serial.println( "Connected" );
    ThingSpeak.begin( client );
}

// Function for Writing Data to ThingSpeak
int write2TSData( long TSChannel, unsigned int TSField1, float field1Data, unsigned int TSField2, float field2Data, unsigned int TSField3, float field3Data , unsigned int TSField4, float field4Data, unsigned int TSField5, float field5Data )
{
  
  ThingSpeak.setField( TSField1, field1Data );
  ThingSpeak.setField( TSField2, field2Data );
  ThingSpeak.setField( TSField3, field3Data );
  ThingSpeak.setField( TSField4, field4Data );
  ThingSpeak.setField( TSField5, field5Data );
   
  int writeSuccess = ThingSpeak.writeFields( TSChannel, writeAPIKey );
  return writeSuccess;
}
