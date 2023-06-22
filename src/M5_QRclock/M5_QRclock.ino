/***************************************************
 M5 QR clock
 Copyright(c) @tomorrow56 All rights reserved.
***************************************************/
#include <M5Stack.h>
//#include "M5StackUpdater.h"

#include <WiFi.h>
#include <TimeLib.h>        // https://forum.arduino.cc/index.php?topic=415296.0

const char* ssid       = "xxxxxxxx";  // your SSID
const char* password   = "xxxxxxxx";  // your Password
IPAddress ipadr;

const char* ntpServer = "ntp.nict.jp";
const long  gmtOffset_sec = 9 * 3600;  // JST = UTC + 9
const int   daylightOffset_sec = 0;
int hh, mm, ss;
int yy, mon, dd;
int retry = 5;
int last_min = 0;
int last_sec = 0;

void setup(){ 
  // init lcd, serial, sd card
  M5.begin(true, true, true);
  M5.Power.begin();

/*
  if(digitalRead(BUTTON_A_PIN) == 0){
    Serial.println("Will Load menu binary");
    updateFromFS(SD);
    ESP.restart();
  }
*/

  M5.Lcd.setBrightness(128);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);

  //Serial.begin(115200);
  Serial.println("M5 QRclock");
  M5.Lcd.println("M5 QRclock");

  M5.Lcd.print("Connect to " + (String)ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      M5.Lcd.print(".");
  }
  M5.Lcd.println(" CONNECTED");

  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.println();
  M5.Lcd.println("SSID:" + (String)WiFi.SSID());
  ipadr = WiFi.localIP();
  M5.Lcd.println("IP adrs: " + (String)ipadr[0] + "." + (String)ipadr[1] + "." + (String)ipadr[2] + "." + (String)ipadr[3]);
  M5.Lcd.println();

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setCursor(10, 10);

  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  struct tm timeinfo;
// int tm_sec;   /* 秒 － [0, 60/61] */
// int tm_min;   /* 分 － [0, 59] */
// int tm_hour;  /* 時 － [0, 23] */
// int tm_mday;  /* 日 － [1, 31] */
// int tm_mon;   /* 1月からの月数 － [0, 11] */
// int tm_year;  /* 1900年からの年数 */
// int tm_wday;  /* 日曜日からの日数 － [0, 6] */
// int tm_yday;  /* 1月1日からの日数 － [0, 365] */
// int tm_isdst; /* 夏時間フラグ */

  for(int i = 0; i < retry; i++){
    if(!getLocalTime(&timeinfo)){
      M5.Lcd.setTextColor(RED);
      Serial.println("Failed to obtain time");
      M5.Lcd.println("Failed to obtain time");
      if(i == retry - 1){
        return;
      }
    }else{
      M5.Lcd.setTextColor(GREEN);
      Serial.println("Connected to NTP Server!");
      M5.Lcd.println("Connected to NTP Server!");
      break;
    }
  }
  yy = 1900 + timeinfo.tm_year;
  mon = timeinfo.tm_mon + 1;
  dd = timeinfo.tm_mday;
 
  hh = timeinfo.tm_hour;
  mm = timeinfo.tm_min;
  ss = timeinfo.tm_sec;

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  // Set current time only the first to values, hh,mm are needed
  // If set the current time to 14:27:00, December 14th, 2015
  // setTime(14, 27, 00, 14, 12, 2015);
  setTime(hh, mm, ss, dd, mon, yy);

  Serial.printf("%04d-%02d-%02d %02d:%02d:%02d\n", yy, mon, dd, hh, mm, ss);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println();
  M5.Lcd.printf("%04d-%02d-%02d %02d:%02d:%02d\n", yy, mon, dd, hh, mm, ss);

//  Serial.printf("%02d:%02d:%02d\n", hh, mm, ss);
//  M5.Lcd.printf("%02d:%02d:%02d\n", hh, mm, ss);

  M5.Lcd.setTextColor(WHITE);

  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setBrightness(127);
}

void loop(){ 
  int i = 0; 
  String NowTime = "";

  if (last_min != minute()){
//  if (last_sec != second()){
//    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.fillRect(60, 20, 200, 200, TFT_WHITE);

    NowTime = String(year()) + "-";

    i = 0;
    while ((i+1)*10 <= month()){
      i++;
    }

    if(i == 0){
      NowTime = NowTime + "0" + String(month()) + "-";
    }else{
      NowTime = NowTime + String(month()) + "-";
    }

    i = 0;
    while ((i+1)*10 <= day()){
      i++;
    }

    if(i == 0){
      NowTime = NowTime + "0" + String(day()) + " ";
    }else{
      NowTime = NowTime + String(day()) + " ";
    }

    i = 0;
    while ((i+1)*10 <= hour()){
      i++;
    }

    if(i == 0){
      NowTime = NowTime + "0" + String(hour()) + ":";
    }else{
      NowTime = NowTime + String(hour()) + ":";
    }

    i = 0;
    while ((i+1)*10 <= minute()){
      i++;
    }

    if(i == 0){
      NowTime = NowTime + "0" + String(minute()) + ":";
    }else{
      NowTime = NowTime + String(minute()) + ":";
    }

    i = 0;
    while ((i+1)*10 <= second()){
      i++;
    }

    if(i == 0){
      NowTime = NowTime + "0" + String(second());
    }else{
      NowTime = NowTime + String(second());
    }

    Serial.println(NowTime);
    M5.Lcd.qrcode(NowTime, 70, 30, 180, 2);

    last_min = minute();
//    last_sec = second();
  }
} 
