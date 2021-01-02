// Konek wifi - read kartu - logging waktu tapping

#include <NTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include "ESP8266HTTPClient.h"
#include "ESP8266WiFi.h"
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>


const char* ssid = "[your_ssid_here]";
const char* password = "[your_wifi_password_here]";
const long utcOffsetInSeconds = 25200;

#define BUZZER          D8
#define LED_G           D1
#define LED_R           D2
#define RST_PIN         0           // Configurable, see typical pin layout above
#define SS_PIN          2          // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
HTTPClient http_put, http_get, http_post,http_get_flag,http_put_flag; 
String formattedDate;
String dayStamp;
String timeStamp;
String daytimeStamp;
String koma = ", ";



//*****************************************************************************************//
void setup() {
  Serial.begin(9600);                                           // Initialize serial communications with the PC
  SPI.begin();                                                  // Init SPI bus
  mfrc522.PCD_Init();                                              // Init MFRC522 card
  pinMode(LED_G, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(D8, OUTPUT);
  WiFi.begin(ssid, password); 
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
 
  Serial.println("Connected to the WiFi network");
  digitalWrite(D1, HIGH);
  tone(D8, 700);
  delay(200);
  tone(D8, 900);
  delay(200);
  tone(D8, 1100);
  delay(500);
  noTone(D8);
  digitalWrite(D1, LOW);
  
  Serial.println("Tap your card");
      //shows in serial that it is ready to read
  timeClient.begin();
}

//*****************************************************************************************//
void loop() {
  if(WiFi.status()== WL_CONNECTED){
  
  
  timeClient.update(); //waktu berjalan
  formattedDate = timeClient.getFormattedDate();
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  timeStamp = timeClient.getFormattedTime();
  daytimeStamp = dayStamp + koma + timeStamp;
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  //some variables we need
  byte block;
  byte len;
  MFRC522::StatusCode status;
  
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  String uid;
  // gabungin uid jadi stirng
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if(mfrc522.uid.uidByte[i] < 0x10){
      uid.concat("0");
    }
    uid.concat(String(mfrc522.uid.uidByte[i],HEX));
  } 

  Serial.print(F("Card Detected with UID:"));
  Serial.println(uid);
  Serial.print(F("UID Logical: "));
  
  

  byte buffer1[60];
  block = 2;
  //Get UID Logical
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 2, &key, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    Serial.println("test");
    digitalWrite(LED_R,HIGH);
    tone(BUZZER, 300);
    delay(500);
    noTone(BUZZER);
    delay(500);
    digitalWrite(LED_R, LOW);
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }

  status = mfrc522.MIFARE_Read(block, buffer1, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    digitalWrite(LED_R,HIGH);
    tone(BUZZER, 300);
    delay(500);
    noTone(BUZZER);
    delay(500);
    digitalWrite(LED_R, LOW);
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }
  String logical_uid;
  char temp;
 
  //PRINT uid logical
  for (uint8_t i = 0; i < 16; i++) {
    if(buffer1[i] == 10){
     continue;
    }
    if(buffer1[i] == 32){
     break;
    }
    temp = buffer1[i]; 
    logical_uid.concat(temp);
  }
  
  //String strip = "-";
  //String logic_uid = uid+strip+logical_id;
  Serial.println(logical_uid);

  //Get Quantity
  Serial.print(F("Quantity: "));
  byte buffer2[20];
  block = 1;

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &key, &(mfrc522.uid)); //line 834
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    digitalWrite(LED_R,HIGH);
    tone(D8, 100);
    delay(500);
    noTone(D8);
    delay(500);
    digitalWrite(LED_R, LOW);
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }

  status = mfrc522.MIFARE_Read(block, buffer2, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    digitalWrite(LED_R,HIGH);
    tone(D8, 100);
    delay(500);
    noTone(D8);
    delay(500);
    digitalWrite(LED_R, LOW);
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }

  String input_quantity; // jumlah barang yang ingin ditambah/dikurang
  int qty; // convert dr string ke int buat quantity
  for (uint8_t i = 0; i < 16; i++) {
    if(buffer2[i] == 10){
     continue;
    }
    if(buffer2[i] == 32){
     break;
    }
    input_quantity.concat(buffer2[i]-48);
  }
  qty = input_quantity.toInt();
  Serial.println(qty);
  Serial.print("DATE: ");
  Serial.println(dayStamp);
  Serial.print("TIME: ");
  Serial.println(timeStamp);
  digitalWrite(D1, HIGH);
  tone(D8, 500);
  delay(500);
  noTone(D8);
  delay(500);
  digitalWrite(D1, LOW);


  // ===================== Section GET =============================
  int get_qty;
  String link_inventory = "http://smpnapi.herokuapp.com/api/v3/inventory/qty/";
  String link_get = link_inventory + logical_uid;
  http_get.begin(link_get);
  http_get.addHeader("Content-Type","application/json");
  http_get.addHeader("Authorization","Basic ZmlsaXA6c2FudG95YWtvYnVz");  
  int httpGetQty = http_get.GET();
  String box = ""; // penyimpan quantity dari GET Json
  
  
  if (httpGetQty == 200) { //Check the returning code
    Serial.print("HTTP Response code for Get: ");
    Serial.println(httpGetQty);
    
    String payload = http_get.getString();   //Get the request response payload
    
    int a = payload.length(); 
 
    for(int i=0; i<a; i++){
      // ngecek payload digit ato bukan, kalo iya di concat ke variabel box
        if(isDigit(payload[i])){ 
          box.concat(payload[i]);
        }else{
          continue;
        }
      }
      Serial.print("get qty = ");  
      int get_q = box.toInt();
      Serial.println(get_q);
      digitalWrite(D1, HIGH);
      tone(D8, 500);
      delay(500);
      noTone(D8);
      delay(500);
      digitalWrite(D1, LOW);
    }
    else{
      Serial.print("HTTP Response code for Get: ");
      Serial.println(httpGetQty);
      digitalWrite(LED_R,HIGH);
      tone(D8, 100);
      delay(500);
      noTone(D8);
      delay(500);
      digitalWrite(LED_R, LOW);
    }
  get_qty = box.toInt();
  // =================================================================
  
  // ===================== Section POST =============================
  
  String link_post = "http://smpnapi.herokuapp.com/api/v3/logging/";
  http_post.begin(link_post);
    http_post.addHeader("Content-Type","application/json");
    http_post.addHeader("Authorization","Basic ZmlsaXA6c2FudG95YWtvYnVz"); 
    http_post.addHeader("Accept","*/*");
    int httpPostCode = http_post.POST("{\"logging\":{\"logical_uid\":\""+logical_uid+"\",\"status\":\"in\",\"qty\":\""+qty+"\",\"time\": \""+daytimeStamp+"\",\"warehouse\":\"warehouse filip\"}}");
    if(httpPostCode == 200){
      Serial.print("HTTP Response code for Logging: ");
      Serial.println(httpPostCode);
      digitalWrite(D1, HIGH);
      tone(D8, 500);
      delay(500);
      noTone(D8);
      delay(500);
      digitalWrite(D1, LOW);
    }
    else{
      Serial.print("HTTP Response code for Logging: ");
      Serial.println(httpPostCode);
      digitalWrite(LED_R,HIGH);
      tone(D8, 100);
      delay(500);
      noTone(D8);
      delay(500);
      digitalWrite(LED_R, LOW);
    }
    
  
  // =================================================================

  // ===================== Section PUT =============================
  int qty_plus = get_qty + qty;
  String link_update = "http://smpnapi.herokuapp.com/api/v3/inventory/update/hardware/";
  String link_put = link_update + logical_uid;
  
  http_put.begin(link_put);
    http_put.addHeader("Content-Type","application/json");
    http_put.addHeader("Authorization","Basic ZmlsaXA6c2FudG95YWtvYnVz");
    http_put.addHeader("Accept","*/*"); 
    
  
    String linkPut = "{\"logical_uid\":\""+logical_uid+"\",\"qty\" : \""+qty_plus+"\"}";
    
    int httpPutCode = http_put.PUT(linkPut);
    String response = http_put.getString();
    if(httpPutCode == 200){  
        Serial.print("HTTP Response code for Updating: ");       
        Serial.println(httpPutCode);
        Serial.println(response);
        digitalWrite(D1, HIGH);
        tone(D8, 500);
        delay(500);
        noTone(D8);
        delay(500);
        digitalWrite(D1, LOW);
    }else{
      Serial.print("HTTP Response code for Updating: ");
      Serial.println(httpPutCode);
      Serial.println(response);
      digitalWrite(LED_R,HIGH);
      tone(D8, 100);
      delay(500);
      noTone(D8);
      delay(500);
      digitalWrite(LED_R, LOW);
    }
    http_put.end();
  
  

  // =================================================================
  
// Indikator berhasil (Hijau) kalau berhasil ke detect kartunya

  if (httpPutCode == 200 && httpPostCode == 200 && httpGetQty == 200){
    tone(D8, 700);
    delay(200);
    tone(D8, 900);
    delay(200);
    tone(D8, 1100);
    delay(500);
    noTone(D8);
  }
  else{
    tone(D8, 500);
    delay(200);
    tone(D8, 300);
    delay(200);
    tone(D8, 100);
    delay(500);
    noTone(D8);
  }

  Serial.println(F("\n**End Reading**\n"));
  
  delay(1000); //change value if you want to read cards faster

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  }else{
    Serial.println("Error in WiFi connection");
    tone(D8, 500);
    delay(200);
    tone(D8, 300);
    delay(200);
    tone(D8, 100);
    delay(500);
    noTone(D8);
  }
  delay(1000);
  return;
}
//*****************************************************************************************//
