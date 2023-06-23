/***************************************************
 M5 QR clock with M5unified
 Copyright(c) @tomorrow56 All rights reserved.
***************************************************/
#if defined ( ARDUINO )
#include <Arduino.h>
#include <SD.h>
#include <SPIFFS.h>
#endif

// Include this to enable the M5 global instance.
#include <M5Unified.h>      // https://github.com/m5stack/M5Unified
#include <esp_log.h>

#include <WiFi.h>
#include <TimeLib.h>        // https://forum.arduino.cc/index.php?topic=415296.0

const char* ssid       = "xxxxxxxx";    // your SSID
const char* password   = "xxxxxxxx";    // your password
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
  auto cfg = M5.config();

#if defined ( ARDUINO )
  cfg.serial_baudrate = 115200;   // default=115200. if "Serial" is not needed, set it to 0.
#endif
  cfg.clear_display = true;  // default=true. clear the screen when begin.
  cfg.output_power  = false;  // default=true. use external port 5V output.
  cfg.internal_imu  = false;  // default=true. use internal IMU.
  cfg.internal_rtc  = true;  // default=true. use internal RTC.
  cfg.internal_spk  = false;  // default=true. use internal speaker.
  cfg.internal_mic  = false;  // default=true. use internal microphone.
  cfg.external_imu  = false;  // default=false. use Unit Accel & Gyro.
  cfg.external_rtc  = false;  // default=false. use Unit RTC.
  cfg.led_brightness = 0;   // default= 0. system LED brightness (0=off / 255=max) (※ not NeoPixel)

  // external speaker setting.
  cfg.external_speaker.module_display = false;  // default=false. use ModuleDisplay AudioOutput
  cfg.external_speaker.hat_spk        = false;  // default=false. use HAT SPK
  cfg.external_speaker.hat_spk2       = false;  // default=false. use HAT SPK2
  cfg.external_speaker.atomic_spk     = false;  // default=false. use ATOMIC SPK
  cfg.external_speaker.module_rca     = false; // default=false. use ModuleRCA AudioOutput

  // external display setting. (Pre-include required)
  cfg.external_display.module_display = false;  // default=true. use ModuleDisplay
  cfg.external_display.atom_display   = false;  // default=true. use AtomDisplay
  cfg.external_display.unit_glass     = false;  // default=true. use UnitGLASS
  cfg.external_display.unit_oled      = false;  // default=true. use UnitOLED
  cfg.external_display.unit_lcd       = false;  // default=true. use UnitLCD
  cfg.external_display.unit_rca       = false;  // default=true. use UnitRCA VideoOutput
  cfg.external_display.module_rca     = false;  // default=true. use ModuleRCA VideoOutput

  // begin M5Unified.
  M5.begin(cfg);

  M5.Display.setBrightness(128);
  M5.Display.setCursor(0, M5.Display.height()/24);
  M5.Display.setTextColor(WHITE);
  M5.Display.setTextSize(2);

  const char* name;

  switch (M5.getBoard()){
#if defined (CONFIG_IDF_TARGET_ESP32S3)
  case m5::board_t::board_M5StackCoreS3:
    name = "StackS3";
    break;
  case m5::board_t::board_M5AtomS3Lite:
    name = "ATOMS3Lite";
    break;
  case m5::board_t::board_M5AtomS3:
    name = "ATOMS3";
    break;
#elif defined (CONFIG_IDF_TARGET_ESP32C3)
  case m5::board_t::board_M5StampC3:
    name = "StampC3";
    break;
  case m5::board_t::board_M5StampC3U:
    name = "StampC3U";
    break;
#else
  case m5::board_t::board_M5Stack:
    name = "Stack";
    break;
  case m5::board_t::board_M5StackCore2:
    name = "StackCore2";
    break;
  case m5::board_t::board_M5StickC:
    name = "StickC";
    break;
  case m5::board_t::board_M5StickCPlus:
    name = "StickCPlus";
    break;
  case m5::board_t::board_M5StackCoreInk:
    name = "CoreInk";
    break;
  case m5::board_t::board_M5Paper:
    name = "Paper";
    break;
  case m5::board_t::board_M5Tough:
    name = "Tough";
    break;
  case m5::board_t::board_M5Station:
    name = "Station";
    break;
  case m5::board_t::board_M5Atom:
    name = "ATOM";
    break;
  case m5::board_t::board_M5AtomPsram:
    name = "ATOM PSRAM";
    break;
  case m5::board_t::board_M5AtomU:
    name = "ATOM U";
    break;
  case m5::board_t::board_M5TimerCam:
    name = "TimerCamera";
    break;
  case m5::board_t::board_M5StampPico:
    name = "StampPico";
    break;
#endif
  default:
    name = "Who am I ?";
    break;
  }

  M5.Display.startWrite();
  M5.Display.print("Core:");
  M5.Display.println(name);
  ESP_LOGI("setup", "core:%s", name);

  // run-time branch : imu model check
  switch (M5.Imu.getType())
  {
  case m5::imu_t::imu_mpu6050:
    name = "MPU6050";
    break;
  case m5::imu_t::imu_mpu6886:
    name = "MPU6886";
    break;
  case m5::imu_t::imu_mpu9250:
    name = "MPU9250";
    break;
  case m5::imu_t::imu_sh200q:
    name = "SH200Q";
    break;
  default:
    name = "none";
    break;
  }
  M5.Display.print("IMU:");
  M5.Display.println(name);
//  M5.Display.endWrite();
  ESP_LOGI("setup", "imu:%s", name);

  Serial.println("M5 QRclock");
  M5.Display.println("M5 QRclock");

  M5.Display.print("Connect to " + (String)ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      M5.Display.print(".");
  }
  M5.Display.println(" CONNECTED");

  M5.Display.setTextColor(GREEN);
  M5.Display.println();
  M5.Display.println("SSID:" + (String)WiFi.SSID());
  ipadr = WiFi.localIP();
  M5.Display.println("IP adrs: " + (String)ipadr[0] + "." + (String)ipadr[1] + "." + (String)ipadr[2] + "." + (String)ipadr[3]);
  M5.Display.println();

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
      M5.Display.setTextColor(RED);
      Serial.println("Failed to obtain time");
      M5.Display.println("Failed to obtain time");
      if(i == retry - 1){
        return;
      }
    }else{
      M5.Display.setTextColor(GREEN);
      Serial.println("Connected to NTP Server!");
      M5.Display.println("Connected to NTP Server!");
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
  M5.Display.setTextColor(WHITE);
  M5.Display.println();
  M5.Display.printf("%04d-%02d-%02d %02d:%02d:%02d\n", yy, mon, dd, hh, mm, ss);

  M5.Display.fillScreen(TFT_BLACK);
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


    switch (M5.getBoard()){
      case m5::board_t::board_M5StackCoreS3:
      case m5::board_t::board_M5Stack:
      case m5::board_t::board_M5StackCore2:
       // 320 x 240
        M5.Display.fillRect(40, 0, 240, 240, TFT_WHITE);
        M5.Display.qrcode(NowTime, 50, 10, 220, 2);
        break;
      case m5::board_t::board_M5AtomS3:
        // 128 x 128
        M5.Display.fillRect(0, 0, 128, 128, TFT_WHITE);
        M5.Display.qrcode(NowTime, 1, 1, 126, 2);
        break;
      case m5::board_t::board_M5StickC:
        // 80 x 160
        M5.Display.fillRect(0, 40, 80, 80, TFT_WHITE);
        M5.Display.qrcode(NowTime, 2, 42, 76, 2);
        break;
      case m5::board_t::board_M5StickCPlus:
        // 135 x 240
        M5.Display.fillRect(0, 52, 135, 135, TFT_WHITE);
        M5.Display.qrcode(NowTime, 4, 56, 127, 2);
        break;
      default:
        break;
    }

    last_min = minute();
    last_sec = second();
  }
} 
