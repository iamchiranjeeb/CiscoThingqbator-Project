#include "OneWire.h"
#include "DallasTemperature.h"
OneWire oneWire(4);
DallasTemperature tempSensor(&oneWire);

#include <Arduino.h>
#include "ESP32_MailClient.h"
#include "SD.h"

#define WIFI_SSID "TQB"  //Wifi Name
#define WIFI_PASSWORD "1234567890" //Wifi Password
HTTPClientESP32Ex http; //WiFi or HTTP client for internet connection
SMTPData smtpData; //The Email Sending data object contains config and data to send
void sendCallback(SendStatus info); //Callback function to get the Email sending status

#include <Wire.h>
#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header
hd44780_I2Cexp lcd;
const int LCD_COLS = 16;
const int LCD_ROWS = 2;




int UpperThreshold = 2300;
    int LowerThreshold = 2100;
    int reading = 0;
    float BPM = 0.0;
    bool IgnoreReading = false;
    bool FirstPulseDetected = false;
    unsigned long FirstPulseTime = 0;
    unsigned long SecondPulseTime = 0;
    unsigned long PulseInterval = 0;


void setup(){
  Serial.begin(115200);
  tempSensor.begin();

  Serial.println();
  Serial.print("Connecting to AP");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(200);
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Serial.println("Mounting SD Card...");

  if (SD.begin()) // MailClient.sdBegin(14,2,15,13) for TTGO T8 v1.7 or 1.8
  {

    Serial.println("Preparing attach file...");

    File file = SD.open("/text_file.txt", FILE_WRITE);
    file.print("Hello World!\r\nHello World!");
    file.close();

    file = SD.open("/binary_file.dat", FILE_WRITE);

    static uint8_t buf[512];

    buf[0] = 'H';
    buf[1] = 'E';
    buf[2] = 'A';
    buf[3] = 'D';
    file.write(buf, 4);

    size_t i;
    memset(buf, 0xff, 512);
    for (i = 0; i < 2048; i++)
    {
      file.write(buf, 512);
    }

    buf[0] = 'T';
    buf[1] = 'A';
    buf[2] = 'I';
    buf[3] = 'L';
    file.write(buf, 4);

    file.close();
  }
  else
  {
    Serial.println("SD Card Monting Failed");
  }

  Serial.println();
  Serial.println("Sending email...");
  //Set the Email host, port, account and password
  smtpData.setLogin("smtp.gmail.com", 465, "projectcisco2019@gmail.com", "myproject01");
  //Set the sender name and Email
  smtpData.setSender("ESP32", "projectcisco2019@gmail.com");
  //Set Email priority or importance High, Normal, Low or 1 to 5 (1 is highest)
  smtpData.setPriority("High");
  //Set the subject
  smtpData.setSubject("Code Trail");
  //Set the message - normal text or html format
  smtpData.setMessage("looser", true);
  //Add recipients, can add more than one recipient
  smtpData.addRecipient("prustysonali98@gmail.com");
  smtpData.setFileStorageType(MailClientStorageType::SD);
  smtpData.setSendCallback(sendCallback);
  //Start sending Email, can be set callback function to track the status
  if (!MailClient.sendMail(http, smtpData))
    Serial.println("Error sending Email, " + MailClient.smtpErrorReason());

  //Clear all data from Email object to free memory
  smtpData.empty();

  pinMode(18,INPUT); //Setup For leads off detecion of LO+
  pinMode(19,INPUT); //Setup For leads off detection of LO-

  int status;

  status = lcd.begin(LCD_COLS, LCD_ROWS);
  if(status) // non zero status means it was unsuccesful
  {
    status = -status; // convert negative status value to positive number

    // begin() failed so blink error code using the onboard LED if possible
    hd44780::fatalError(status); // does not return
  }

  // Print a message to the LCD
  //lcd.print("Hello, World!");

}

void loop(){
   tempSensor.requestTemperaturesByIndex(0);
  
  Serial.print("Temperature: ");
  Serial.print(tempSensor.getTempCByIndex(0));
  Serial.println(" C");

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Temp.");
  lcd.print(tempSensor.getTempCByIndex(0));
  lcd.print("C");


  //delay(2000);

  if(tempSensor.getTempCByIndex(0) > 25)
  {
    Serial.println("Temp Exceeding");
    Serial.println("Sending email...");
    smtpData.setLogin("smtp.gmail.com", 465, "projectcisco2019@gmail.com", "myproject01");
    smtpData.setSender("Emergency", "Data From Patient");
    smtpData.setPriority("High");
    smtpData.setSubject("Temprature");
    smtpData.setMessage("Contact To The Patient ASAP,Temp Exceeding", true);
    smtpData.addRecipient("chandansahoo438@gmail.com");
    smtpData.setFileStorageType(MailClientStorageType::SD);
    smtpData.setSendCallback(sendCallback);
     if (!MailClient.sendMail(http, smtpData))
    Serial.println("Error sending Email, " + MailClient.smtpErrorReason());

  //Clear all data from Email object to free memory
    smtpData.empty();

  tempSensor.begin();
  }

  if(tempSensor.getTempCByIndex(0) > 25)
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Temp Going Up");
    
  }
  delay(2000);

  reading = analogRead(34); 

      // Heart beat leading edge detected.
      if(reading > UpperThreshold && IgnoreReading == false){
        if(FirstPulseDetected == false){
          FirstPulseTime = millis();
          FirstPulseDetected = true;
        }
        else{
          SecondPulseTime = millis();
          PulseInterval = SecondPulseTime - FirstPulseTime;
          FirstPulseTime = SecondPulseTime;
        }
        IgnoreReading = true;
      }

      // Heart beat trailing edge detected.
      if(reading < LowerThreshold){
        IgnoreReading = false;
      }  

      BPM = (1.0/PulseInterval) * 60.0 * 1000;

      Serial.print(reading);
      Serial.print("\t");
      Serial.print(PulseInterval);
      Serial.print("\t");
      Serial.print(BPM);
      Serial.println(" BPM");
      Serial.flush();

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(BPM);
      lcd.print("BPM");

      if(BPM>20)
      {
        Serial.println("BPM Going High");
        smtpData.setLogin("smtp.gmail.com", 465, "projectcisco2019@gmail.com", "myproject01");
        smtpData.setSender("Emergency", "Data From Patient");
        smtpData.setPriority("High");
        smtpData.setSubject("BPM");
        smtpData.setMessage("Contact To The patient,BPM Exceeding", true);
        smtpData.addRecipient("chandansahoo438@gmail.com");
        smtpData.setFileStorageType(MailClientStorageType::SD);
        smtpData.setSendCallback(sendCallback);
      if (!MailClient.sendMail(http, smtpData))
       Serial.println("Error sending Email, " + MailClient.smtpErrorReason());
       smtpData.empty();
   
      }

      if(BPM>20)
      {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("BPM Going High");
      }
      delay(1100);

      // Please don't use delay() - this is just for testing purposes.
      //delay(50);  

      if((digitalRead(18) == 1)||(digitalRead(19) == 1)){
    Serial.println('!');
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Can't Read Value");
    
  }
  else{
    Serial.println(analogRead(35));
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(analogRead(35));
  }
  delay(1000);
  
}
//Callback function to get the Email sending status
void sendCallback(SendStatus msg)
{
  //Print the current status
  Serial.println(msg.info());

  //Do something when complete
  if (msg.success())
  {
    Serial.println("----------------");
  }
}
