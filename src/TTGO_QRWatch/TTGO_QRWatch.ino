/***************************************************
 QR Watch for TTGO T-Watch(LovyanGFX version)
 Copyright(c) @tomorrow56 All rights reserved.
***************************************************/
#if defined ( ARDUINO )
#include <Arduino.h>
#include <SD.h>
#include <SPIFFS.h>
#endif

// #define LGFX_TTGO_TWATCH                   // TTGO T-Watch
#define LGFX_AUTODETECT // 自動認識

#include "qrcode.h"

#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>  // クラス"LGFX"を用意します

static LGFX lcd;                 // LGFXのインスタンスを作成。
// static LGFX_Sprite sprite(&lcd); // スプライトを使う場合はLGFX_Spriteのインスタンスを作成。

#include <WiFi.h>
#include <TimeLib.h>        // https://forum.arduino.cc/index.php?topic=415296.0

const char* ssid       = "xxxxxxxx";  // your Wi-Fi SSID
const char* password   = "xxxxxxxx";  // your Wi-Fi password
IPAddress ipadr;

const char* ntpServer = "ntp.nict.jp";
const long  gmtOffset_sec = 9 * 3600;  // JST = UTC + 9
const int   daylightOffset_sec = 0;
int hh, mm, ss;
int yy, mon, dd;
int retry = 5;
int last_min = 0;
int last_sec = 0;

// LCD 1.54' 240 x 240
int lcd_w = 240;
int lcd_h = 240;

int qr_size = 0;
int qr_margin = 0;

void setup(){ 

  Serial.begin(115200);

  lcd.init();
  lcd.setRotation(0);
  lcd.setBrightness(128);
  lcd.setColorDepth(16);  // RGB565の16ビットに設定
//  lcd.setColorDepth(24);  // RGB888の24ビットに設定(表示される色数はパネル性能によりRGB666の18ビットになります)

  lcd.setCursor(0, lcd_h / 24);
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(1);

  const char* name = "TTGO T-Watch";

  lcd.startWrite();
  lcd.print("Core:");
  lcd.println(name);

  Serial.println("TTGO QR-Watch");
  lcd.println("TTGO QR-Watch");

  Serial.print("Connect to " + (String)ssid);
  lcd.print("Connect to " + (String)ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      lcd.print(".");
  }
  Serial.println(" CONNECTED");
  lcd.println(" CONNECTED");

  lcd.setTextColor(TFT_GREEN);
  lcd.println();
  lcd.println("SSID:" + (String)WiFi.SSID());
  ipadr = WiFi.localIP();
  Serial.println("IP adrs: " + (String)ipadr[0] + "." + (String)ipadr[1] + "." + (String)ipadr[2] + "." + (String)ipadr[3]);
  Serial.println();
  lcd.println("IP adrs: " + (String)ipadr[0] + "." + (String)ipadr[1] + "." + (String)ipadr[2] + "." + (String)ipadr[3]);
  lcd.println();

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
      lcd.setTextColor(TFT_RED);
      Serial.println("Failed to obtain time");
      lcd.println("Failed to obtain time");
      if(i == retry - 1){
        return;
      }
    }else{
      lcd.setTextColor(TFT_GREEN);
      Serial.println("Connected to NTP Server!");
      lcd.println("Connected to NTP Server!");
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
  lcd.setTextColor(TFT_WHITE);
  lcd.println();
  lcd.printf("%04d-%02d-%02d %02d:%02d:%02d\n", yy, mon, dd, hh, mm, ss);

  lcd.fillScreen(TFT_BLACK);

  if(lcd_w <= lcd_h){
    qr_size = lcd_w;
  }else{
    qr_size = lcd_h;
  }

  lcd.fillRect((lcd_w - qr_size) / 2, (lcd_h - qr_size) / 2, qr_size, qr_size, TFT_WHITE);

  switch(qr_size){
    case 240:
      qr_margin = 5;
      break;
    case 128:
      qr_margin = 1;
      break;
    case 80:
      qr_margin = 2;
      break;
    case 135:
      qr_margin = 4;
      break;
    default:
      qr_margin = 3;
      break;
  }
}

void loop(){ 
  int i = 0; 
  String NowTime = "";

//  if (last_min != minute()){
  if (last_sec != second()){

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
    M5qrcode(NowTime, (lcd_w - qr_size) / 2 + qr_margin, (lcd_h - qr_size) / 2 + qr_margin, qr_size - (qr_margin * 2), 2);

    last_min = minute();
    last_sec = second();
  }
} 

void M5qrcode(const String &string, uint16_t x, uint16_t y, uint8_t width, uint8_t version) {
    int16_t len = string.length() + 2;
    char buffer[len];
    string.toCharArray(buffer, len);

    // Create the QR code
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(version)];
    qrcode_initText(&qrcode, qrcodeData, version, 0, buffer);

    // Top quiet zone
    uint8_t thickness   = width / qrcode.size;
    uint16_t lineLength = qrcode.size * thickness;
    uint8_t xOffset     = x + (width - lineLength) / 2;
    uint8_t yOffset     = y + (width - lineLength) / 2;
//    M5.Display.fillRect(x, y, width, width, TFT_WHITE);

    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            uint8_t q = qrcode_getModule(&qrcode, x, y);
            if (q){
                lcd.fillRect(x * thickness + xOffset, y * thickness + yOffset, thickness, thickness, TFT_BLACK);
            }else{
                lcd.fillRect(x * thickness + xOffset, y * thickness + yOffset, thickness, thickness, TFT_WHITE);
            }
        }
    }
}
