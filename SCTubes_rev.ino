#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h> 
#include <Wire.h>
#include "DHT.h"
#include <MQ2.h>
#include <LiquidCrystal_I2C.h>
#include <FirebaseESP32.h>

// Replace with your network credentials
const char* ssid = "!yesn'ts";
const char* password = ""; 

// Initialize Telegram BOT
#define BOTtoken "5905728552:AAH9nEa6pE7dCt8RLHPC4qqZq-tQ6XFvsQY"  // your Bot Token (Get from Botfather)
 
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
 
// Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

//sensor kelembapan
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float temperature;
float humidity;
//sensor udara
#define PIN_MQ2 39
MQ2 mq2(PIN_MQ2);
float smoke_conc;

//LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// relay_kipas
#define relay_kipas 2

//subcribe
bool sub = true;

// Firebase
#define FIREBASE_HOST "https://tubessister-bfe9e-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_Authorization_key "AIzaSyDdLJ14AhGq7NRK2WmX_8u8Xlj05RW-MwY"

FirebaseData firebaseData;
FirebaseJson json;
FirebaseData fbdo;

//Parameter
float threshold_suhu;
float threshold_asap;
//cek ini kalo 60% dia 0,6 apa 60
const float threshold_lembab = 60;

void kirimPesan(String pesan){
  String chat_id = "979590752";
  bot.sendMessage(chat_id, String(pesan), "");
}

void tampilLCD(){
  lcd.clear(); 
  lcd.setCursor(0, 0);
  lcd.print("Kadar Asap: " + String(smoke_conc));
  lcd.setCursor(2, 0);
  lcd.print("temperaturee: " + String(temperature));
}

void bacaSensorMQ(){

  smoke_conc = mq2.readSmoke();

  Serial.println("kualitas udara: " + String(smoke_conc));

  if(smoke_conc > threshold_asap){
    if(sub){
      kirimPesan("Udara Kotor! Kipas akan Dinyalakan");
    }
    digitalWrite(relay_kipas, HIGH);
  }else{
    digitalWrite(relay_kipas, LOW);
  }

  delay(100);

}

void bacaSensorDHT(){
  //  sensor kelembapan
 
  temperature = dht.readTemperature();
  humidity=dht.readHumidity();
  Serial.println("temperaturee: " + String(temperature));

  if(temperature >= threshold_suhu){
    digitalWrite(relay_kipas, LOW);
  }else{
    digitalWrite(relay_kipas, HIGH);
  }
  if(humidity >= threshold_lembab){
    digitalWrite(relay_kipas, LOW);
  }else{
    digitalWrite(relay_kipas, HIGH);
  }
}
 
void bacaPesan(int numNewMessages) {
 
 Serial.println("bacaPesan");
 Serial.println(String(numNewMessages));
 
 for (int i=0; i<numNewMessages; i++) {
   // Chat id of the requester
   String chat_id = String(bot.messages[i].chat_id);
 
   // Print the received message
   String text = bot.messages[i].text;
   Serial.println(text);
 
   String from_name = bot.messages[i].from_name;

   if (text == "sub") {
     
     sub = !sub;

     if(sub){
       bot.sendMessage(chat_id, "Langganan diaktifkan", "");
     }else{
       bot.sendMessage(chat_id, "Langganan dimatikan", "");
     }
     
   }

   if (text == "nyalakan kipas") {
      digitalWrite(relay_kipas,LOW);
      delay(30000);  
      digitalWrite(relay_kipas,HIGH);
   }
   
 }
}
 
void setup() {
 Serial.begin(115200);
 
 // Connect to Wi-Fi
 WiFi.begin(ssid, password);
 client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
 Serial.print("Connecting to WiFi");
 lcd.setCursor(0, 0);
 lcd.print("Connecting to WiFi");
  
 while (WiFi.status() != WL_CONNECTED) {
   delay(1000);
   Serial.print(".");
   lcd.setCursor(2, 1);
   lcd.print(".");
 }
 Serial.println();
 // Print ESP32 Local IP Address
 Serial.println(WiFi.localIP());

 Firebase.begin(FIREBASE_HOST,FIREBASE_Authorization_key);
 
 lcd.begin();
 lcd.backlight();
 lcd.clear();

 mq2.begin();
 pinMode(relay_kipas, OUTPUT);
}
 
void loop() {

 if (Firebase.ready())
 {
    String hsl_suhu = Firebase.RTDB.getFloat(&fbdo, F("/Parameter/suhu")) ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str();
    threshold_suhu = hsl_suhu.toFloat();
    
    String hsl_asap = Firebase.RTDB.getFloat(&fbdo, F("/Parameter/asap")) ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str();
    threshold_asap = hsl_asap.toFloat();
 }

 if (millis() > lastTimeBotRan + botRequestDelay)  {
   int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

   while(numNewMessages) {
     Serial.println("got response");
     bacaPesan(numNewMessages);
     numNewMessages = bot.getUpdates(bot.last_message_received + 1);
   }

   lastTimeBotRan = millis();
 }

 bacaSensorMQ();
 bacaSensorDHT();
 tampilLCD();

}