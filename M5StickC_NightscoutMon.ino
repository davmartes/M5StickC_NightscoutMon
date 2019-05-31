/*  M5StickC Nightscout monitor
    Copyright (C) 2018, 2019 Martin Lukasek <martin@lukasek.cz>
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>. 
    This software uses some 3rd party libraries:
    IniFile by Steve Marple <stevemarple@googlemail.com> (GNU LGPL v2.1)
    ArduinoJson by Benoit BLANCHON (MIT License) 
*/




#include <M5StickC.h>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include "time.h"
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>
#include "M5StickC_NSconfig.h"

tConfig cfg;

extern const unsigned char WiFi_symbol[];

const char* ntpServer = "pool.ntp.org";
struct tm localTimeInfo;
int lastSec = 61;
char localTimeStr[30];

#ifndef min
  #define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

WiFiMulti WiFiMulti;
unsigned long msCount;
unsigned long msCountLog;
unsigned long msCountAlert;
static uint8_t lcdBrightness = 10;

DynamicJsonDocument JSONdoc(16384);
float last10sgv[10];
int wasError = 0;
time_t lastAlarmTime = 0;
time_t lastSnoozeTime = 0;
int led_alert = 0;

void startupLogo() {
    // static uint8_t brightness, pre_brightness;
    M5.Axp.ScreenBreath(0);
    if(cfg.bootPic[0]==0) {
      // M5.Lcd.pushImage(0, 0, 160, 80, (uint16_t *)gImage_logoM5);
      M5.Lcd.drawString("RENAULT Megane", 36, 10, 2);
      M5.Lcd.drawString("TYRES MONITOR", 36, 22, 2);
      M5.Lcd.drawString("(c) 2019", 55, 50, 2);
    } else {
      // M5.Lcd.drawJpgFile(SD, cfg.bootPic);
    }
    // M5.update();
    // M5.Speaker.playMusic(m5stack_startup_music,25000);

    // char tmpstr[32];
    for(int i=0; i<=15; i++) {
      M5.Axp.ScreenBreath(i);
      // sprintf(tmpstr, "%d", i);
      // M5.Lcd.fillRect(0, 0, 30, 20, TFT_BLACK);
      // M5.Lcd.drawString(tmpstr, 0, 0, GFXFF);
      delay(100);
    }
    delay(2000);
    M5.Lcd.fillScreen(BLACK);
}

void printLocalTime() {
  if(!getLocalTime(&localTimeInfo)){
    Serial.println("TIME");
    M5.Lcd.println("TIME");
    return;
  }
  Serial.println(&localTimeInfo, "%A, %B %d %Y %H:%M:%S");
  M5.Lcd.println(&localTimeInfo, "%A, %B %d %Y %H:%M:%S");
}

void sndAlarm() {
    for(int j=0; j<6; j++) {
      /*
      if( cfg.dev_mode )
        // play_tone(660, 400, 1);
      else
        // play_tone(660, 400, 20); */
      delay(200);
    }
}

void sndWarning() {
  for(int j=0; j<3; j++) {
    /*
    if( cfg.dev_mode )
      // play_tone(3000, 100, 1);
    else
      // play_tone(3000, 100, 100);*/
    delay(300);
  }
}

int tmpvol = 1;
void buttons_test() {
  if(digitalRead(M5_BUTTON_HOME) == LOW){
    // M5.Lcd.printf("A");
    Serial.printf("A");
    // play_tone(1000, 10, 1);
    // sndAlarm();
    Serial.print("Change brightness from "); Serial.print(lcdBrightness);
    if(lcdBrightness==cfg.brightness1) 
      lcdBrightness = cfg.brightness2;
    else
      if(lcdBrightness==cfg.brightness2) 
        lcdBrightness = cfg.brightness3;
      else
        lcdBrightness = cfg.brightness1;
    Serial.print(" to "); Serial.println(lcdBrightness);
    M5.Axp.ScreenBreath(lcdBrightness);
    while(digitalRead(M5_BUTTON_HOME) == LOW); // wait for release
  }
  if(digitalRead(M5_BUTTON_RST) == LOW) {
    digitalWrite(M5_LED, LOW);
    delay(500);
    digitalWrite(M5_LED, HIGH);
    while(digitalRead(M5_BUTTON_RST) == LOW); // wait for release
  }
}

void wifi_connect() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    Serial.println("Buscando Internet");
    M5.Lcd.println("Buscando Internet");

    // We start by connecting to a WiFi network
    if(cfg.wlan1ssid[0]!=0)
      WiFiMulti.addAP(cfg.wlan1ssid, cfg.wlan1pass);
    if(cfg.wlan2ssid[0]!=0)
      WiFiMulti.addAP(cfg.wlan2ssid, cfg.wlan2pass);
    if(cfg.wlan3ssid[0]!=0)
      WiFiMulti.addAP(cfg.wlan3ssid, cfg.wlan3pass);

    Serial.println();
    M5.Lcd.println("");
    Serial.print("Conectandose al WIFI... ");
    M5.Lcd.print("Conectandose al WIFI... ");

    while(WiFiMulti.run() != WL_CONNECTED) {
        Serial.print(".");
        M5.Lcd.print(".");
        delay(500);
    }

    Serial.println("");
    M5.Lcd.println("");
    Serial.println("CONECTADO");
    M5.Lcd.println("CONECTADO");
    Serial.println("IP address: ");
    M5.Lcd.println("IP address: ");
    Serial.println(WiFi.localIP());
    M5.Lcd.println(WiFi.localIP());

    configTime(cfg.timeZone, cfg.dst, ntpServer);
    delay(1000);
    printLocalTime();

    Serial.println("CONECTADO");
    M5.Lcd.println("CONECTADO");
}

void setup() {
  M5.begin();                     
  pinMode(M5_BUTTON_HOME, INPUT);
  pinMode(M5_BUTTON_RST, INPUT);
  pinMode(M5_LED, OUTPUT);
  led_alert = 0;
  digitalWrite(M5_LED, HIGH);
  
  // Lcd display
  M5.Axp.ScreenBreath(0);
  M5.Lcd.setRotation(3);     // SCREEN ORIENTATION 0=cable at the bottom 1=cable on the right 2=? 3=cable on the left 4=? 5=? 6=? 7=? 8=? 9=?

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(0, 0, 1);
  M5.Lcd.setTextSize(1);
  yield();

  Serial.print("Free Heap = "); Serial.println(ESP.getFreeHeap());

  readConfiguration(&cfg);
  // cfg.snd_warning = 5.5;
  // cfg.snd_alarm = 4.5;
  // cfg.snd_warning_high = 9;
  // cfg.snd_alarm_high = 10;
  // cfg.alarm_repeat = 1;
  // cfg.snooze_timeout = 2;
  // cfg.brightness1 = 0;

  startupLogo();
  //M5.Speaker.mute();
  yield();

  /*
  char tmpstr[64];
  for(int i=0; i<=8; i++) {
    M5.Lcd.fillScreen(TFT_BLACK);
    sprintf(tmpstr, "%d JSON parsing failed", i);
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawString(tmpstr, 0, 0, i);
    delay(1000);
  }
  */

  lcdBrightness = cfg.brightness1;
  M5.Axp.ScreenBreath(lcdBrightness);
  wifi_connect();
  yield();

  M5.Axp.ScreenBreath(lcdBrightness);
  M5.Lcd.fillScreen(TFT_BLACK);

  // test file with time stamps
  msCountLog = millis()-6000;
   
  // test file with time stamps
  msCountAlert = millis()-2000;

  // update glycemia now
  msCount = millis()-16000;
}

void drawArrow(int x, int y, int asize, int aangle, int pwidth, int plength, uint16_t color){
  float dx = (asize-10)*cos(aangle-90)*PI/180+x; // calculate X position  
  float dy = (asize-10)*sin(aangle-90)*PI/180+y; // calculate Y position  
  float x1 = 0;         float y1 = plength;
  float x2 = pwidth/2;  float y2 = pwidth/2;
  float x3 = -pwidth/2; float y3 = pwidth/2;
  float angle = aangle*PI/180-135;
  float xx1 = x1*cos(angle)-y1*sin(angle)+dx;
  float yy1 = y1*cos(angle)+x1*sin(angle)+dy;
  float xx2 = x2*cos(angle)-y2*sin(angle)+dx;
  float yy2 = y2*cos(angle)+x2*sin(angle)+dy;
  float xx3 = x3*cos(angle)-y3*sin(angle)+dx;
  float yy3 = y3*cos(angle)+x3*sin(angle)+dy;
  M5.Lcd.fillTriangle(xx1,yy1,xx3,yy3,xx2,yy2, color);
  M5.Lcd.drawLine(x, y, xx1, yy1, color);
  M5.Lcd.drawLine(x+1, y, xx1+1, yy1, color);
  M5.Lcd.drawLine(x, y+1, xx1, yy1+1, color);
  M5.Lcd.drawLine(x-1, y, xx1-1, yy1, color);
  M5.Lcd.drawLine(x, y-1, xx1, yy1-1, color);
  M5.Lcd.drawLine(x+2, y, xx1+2, yy1, color);
  M5.Lcd.drawLine(x, y+2, xx1, yy1+2, color);
  M5.Lcd.drawLine(x-2, y, xx1-2, yy1, color);
  M5.Lcd.drawLine(x, y-2, xx1, yy1-2, color);
}
void update_glycemia() {
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 0);
  // if there was an error, then clear whole screen, otherwise only graphic updated part
  if( wasError ) {
    M5.Lcd.fillScreen(BLACK);
  } else {
    // M5.Lcd.fillRect(230, 110, 90, 100, TFT_BLACK);
//    M5.Lcd.fillRect(96, 16, 64, 48, TFT_BLACK);
    // M5.Lcd.fillScreen(BLACK);
  }
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
//  M5.Lcd.drawBitmap(96, 16, 64, 48, (uint16_t *)WiFi_symbol);
  // uint16_t maxWidth, uint16_t maxHeight, uint16_t offX, uint16_t offY, jpeg_div_t scale);
  if((WiFiMulti.run() == WL_CONNECTED)) {

    HTTPClient http;

    Serial.print(" ");
    // configure target server and url
    char NSurl[128];
    strcpy(NSurl,"https://");
    strcat(NSurl,cfg.url);
    strcat(NSurl,"/api/v1/entries.json");
    // more info at /api/v2/properties
    http.begin(NSurl); //HTTP
    
    Serial.print(" ");
    // start connection and send HTTP header
    int httpCode = http.GET();
  
    // httpCode will be negative on error
    if(httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf(" ", httpCode);

      // file found at server
      if(httpCode == HTTP_CODE_OK) {
        String json = http.getString();
        wasError = 0;
        // Serial.println(json);
        // const size_t capacity = JSON_ARRAY_SIZE(10) + 10*JSON_OBJECT_SIZE(19) + 3840;
        // Serial.print("JSON size needed= "); Serial.print(capacity); 
        Serial.print(" "); Serial.println(ESP.getFreeHeap());
        auto JSONerr = deserializeJson(JSONdoc, json);
        if (JSONerr) {   //Check for errors in parsing
          Serial.println(" ");
          M5.Lcd.drawString(" ", 0, 0, 2);
          wasError = 1;
        } else {
          char sensDev[64];
          uint64_t rawtime = 0;
          char sensDir[32];
          float sensSgv = 0;
          JsonObject obj; 
          int sgvindex = 0;
          do {
            obj=JSONdoc[sgvindex].as<JsonObject>();
            sgvindex++;
          } while ((!obj.containsKey("sgv")) && (sgvindex<9));
          sgvindex--;
          if(sgvindex<0 || sgvindex>8)
            sgvindex=0;
          strlcpy(sensDev, JSONdoc[sgvindex]["device"] | "N/A", 64);
          rawtime = JSONdoc[sgvindex]["date"].as<long long>(); // sensTime is time in milliseconds since 1970, something like 1555229938118
          strlcpy(sensDir, JSONdoc[sgvindex]["direction"] | "N/A", 32);
          sensSgv = JSONdoc[sgvindex]["sgv"]; // get value of sensor measurement
          time_t sensTime = rawtime / 1000; // no milliseconds, since 2000 would be - 946684800, but ok
          for(int i=0; i<=9; i++) {
            last10sgv[i]=JSONdoc[i]["sgv"];
            last10sgv[i]/=18.0;
          }
          float sensSgvMgDl = sensSgv;
          // internally we work in mmol/L
          sensSgv/=18.0;
          
          char tmpstr[255];
          struct tm sensTm;
          localtime_r(&sensTime, &sensTm);
          
          Serial.print("sensDev = ");
          Serial.println(sensDev);
          Serial.print("sensTime = ");
          Serial.print(sensTime);
          sprintf(tmpstr, " (JSON %lld)", (long long) rawtime);
          Serial.print(tmpstr);
          sprintf(tmpstr, " = %s", ctime(&sensTime));
          Serial.print(tmpstr);
          Serial.print("sensSgv = ");
          Serial.println(sensSgv);
          Serial.print("sensDir = ");
          Serial.println(sensDir);
         
          // Serial.print(sensTm.tm_year+1900); Serial.print(" / "); Serial.print(sensTm.tm_mon+1); Serial.print(" / "); Serial.println(sensTm.tm_mday);
          Serial.print("Sensor time: "); Serial.print(sensTm.tm_hour); Serial.print(":"); Serial.print(sensTm.tm_min); Serial.print(":"); Serial.print(sensTm.tm_sec); Serial.print(" DST "); Serial.println(sensTm.tm_isdst);

          M5.Lcd.setTextSize(2);
          M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
          // char dateStr[30];
          // sprintf(dateStr, "%d.%d.%04d", sensTm.tm_mday, sensTm.tm_mon+1, sensTm.tm_year+1900);
          // M5.Lcd.drawString(dateStr, 0, 48, GFXFF);
          // char timeStr[30];
          // sprintf(timeStr, "%02d:%02d:%02d", sensTm.tm_hour, sensTm.tm_min, sensTm.tm_sec);
          // M5.Lcd.drawString(timeStr, 0, 72, GFXFF);
          char datetimeStr[64];
          struct tm timeinfo;
          /*
          if(cfg.show_current_time) {
            if(getLocalTime(&timeinfo)) {
              sprintf(datetimeStr, "%02d:%02d:%02d  %d.%d.  ", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, timeinfo.tm_mday, timeinfo.tm_mon+1);  
              Serial.println(datetimeStr);
            } else {
              strcpy(datetimeStr, "??:??:??");
              Serial.println(datetimeStr);
            }
          } else {
            sprintf(datetimeStr, "%02d:%02d:%02d  %d.%d.  ", sensTm.tm_hour, sensTm.tm_min, sensTm.tm_sec, sensTm.tm_mday, sensTm.tm_mon+1);
            Serial.println(datetimeStr);
          }
          M5.Lcd.drawString(datetimeStr, 0, 0, 2);
          */
          
          // M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
          // M5.Lcd.drawString(cfg.userName, 0, 24, 2);
          
          /*
          if( !cfg.dev_mode ) {
            M5.Lcd.drawString("Nightscout", 0, 48, GFXFF);
          } else {
            char heapstr[20];
            sprintf(heapstr, "%i free  ", ESP.getFreeHeap());
            M5.Lcd.drawString(heapstr, 0, 48, GFXFF);
          }
          */
          
          strcpy(NSurl,"https://");
          strcat(NSurl,cfg.url);
          strcat(NSurl,"/api/v2/properties/iob,cob,delta");
          http.begin(NSurl); //HTTP
          Serial.print("[HTTP] GET properties...\n");
          int httpCode = http.GET();
          if(httpCode > 0) {
            Serial.printf("[HTTP] GET properties... code: %d\n", httpCode);
            if(httpCode == HTTP_CODE_OK) {
              // const char* propjson = "{\"iob\":{\"iob\":0,\"activity\":0,\"source\":\"OpenAPS\",\"device\":\"openaps://Spike iPhone 8 Plus\",\"mills\":1557613521000,\"display\":\"0\",\"displayLine\":\"IOB: 0U\"},\"cob\":{\"cob\":0,\"source\":\"OpenAPS\",\"device\":\"openaps://Spike iPhone 8 Plus\",\"mills\":1557613521000,\"treatmentCOB\":{\"decayedBy\":\"2019-05-11T23:05:00.000Z\",\"isDecaying\":0,\"carbs_hr\":20,\"rawCarbImpact\":0,\"cob\":7,\"lastCarbs\":{\"_id\":\"5cd74c26156712edb4b32455\",\"enteredBy\":\"Martin\",\"eventType\":\"Carb Correction\",\"reason\":\"\",\"carbs\":7,\"duration\":0,\"created_at\":\"2019-05-11T22:24:00.000Z\",\"mills\":1557613440000,\"mgdl\":67}},\"display\":0,\"displayLine\":\"COB: 0g\"},\"delta\":{\"absolute\":-4,\"elapsedMins\":4.999483333333333,\"interpolated\":false,\"mean5MinsAgo\":69,\"mgdl\":-4,\"scaled\":-0.2,\"display\":\"-0.2\",\"previous\":{\"mean\":69,\"last\":69,\"mills\":1557613221946,\"sgvs\":[{\"mgdl\":69,\"mills\":1557613221946,\"device\":\"MIAOMIAO\",\"direction\":\"Flat\",\"filtered\":92588,\"unfiltered\":92588,\"noise\":1,\"rssi\":100}]}}}";
              String propjson = http.getString();
              const size_t propcapacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(6) + 2*JSON_OBJECT_SIZE(7) + 4*JSON_OBJECT_SIZE(8) + 770 + 1000;
              DynamicJsonDocument propdoc(propcapacity);
              auto propJSONerr = deserializeJson(propdoc, propjson);
              if(propJSONerr) {
                Serial.println("Properties JSON parsing failed");
                M5.Lcd.fillRect(0,24,69,23,TFT_BLACK);
                M5.Lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
                M5.Lcd.setTextSize(2);
                M5.Lcd.drawString("???", 115, 0, 2);
              } else {
                JsonObject iob = propdoc["iob"];
                float iob_iob = iob["iob"]; // 0
                const char* iob_display = iob["display"]; // "0"
                const char* iob_displayLine = iob["displayLine"]; // "IOB: 0U"
                
                JsonObject cob = propdoc["cob"];
                float cob_cob = cob["cob"]; // 0
                int cob_display = cob["display"]; // 0
                const char* cob_displayLine = cob["displayLine"]; // "COB: 0g"




//        DELTA SECTION




                
                JsonObject delta = propdoc["delta"];
                int delta_absolute = delta["absolute"]; // -4
                float delta_elapsedMins = delta["elapsedMins"]; // 4.999483333333333
                bool delta_interpolated = delta["interpolated"]; // false
                int delta_mean5MinsAgo = delta["mean5MinsAgo"]; // 69
                int delta_mgdl = delta["mgdl"]; // -4
                float delta_scaled = delta["scaled"]; // -0.2
                const char* delta_display = delta["display"]; // "-0.2"
                // M5.Lcd.fillRect(96,0,64,12,TFT_BLACK);
                M5.Lcd.fillRoundRect(85,0,84,33,0,TFT_BLACK);           // MASK RECTANGLE DELTA
                M5.Lcd.setTextSize(2);
                M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                M5.Lcd.drawString(delta_display, 110, 0, 2);
      
                if(cfg.show_COB_IOB) {
                  M5.Lcd.fillRect(0,70,96,10,TFT_BLACK);
                  if(iob_iob>0)
                    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                  else
                    M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
                  M5.Lcd.drawString(iob_displayLine, 0, 71, 1);
                  if(cob_cob>0)
                    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
                  else
                    M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
                  M5.Lcd.drawString(cob_displayLine, 60, 71, 1);
                }
              }
            }
          }





//      TIME AGO SECTION



    
          // calculate sensor time difference
          int sensorDifSec=0;
          if(!getLocalTime(&timeinfo)){ // aaa - have to look at it later, sometime it fails here
            sensorDifSec=24*60*60; // too much
          } else {
            Serial.print("Local time: "); Serial.print(timeinfo.tm_hour); Serial.print(":"); Serial.print(timeinfo.tm_min); Serial.print(":"); Serial.print(timeinfo.tm_sec); Serial.print(" DST "); Serial.println(timeinfo.tm_isdst);
            sensorDifSec=difftime(mktime(&timeinfo), sensTime);
          }
          Serial.print("Sensor time difference = "); Serial.print(sensorDifSec); Serial.println(" sec");
          unsigned int sensorDifMin = (sensorDifSec+30)/60;
          uint16_t tdColor = TFT_BLACK;
          if(sensorDifMin>5) {
            tdColor = TFT_MAGENTA;
            if(sensorDifMin>15) {
              tdColor = TFT_RED;
            }
          }
          
          M5.Lcd.fillRoundRect(0,0,84,33,0,tdColor);           // RECTANGLE TIME AGO
          M5.Lcd.setTextSize(2);
          // M5.Lcd.setTextDatum(MC_DATUM);
          M5.Lcd.setTextColor(TFT_WHITE, tdColor);
          if(sensorDifMin>99) {
            M5.Lcd.drawString("XXX", 15, 0, 2);
          } else {
            char tmpstr[32];
            sprintf(tmpstr, " %d", sensorDifMin);
            M5.Lcd.drawString(tmpstr, 19, 0, 2);
          }






//      BG SECTION



          
          uint16_t glColor = TFT_GREEN;
          if(sensSgv<cfg.yellow_low || sensSgv>cfg.yellow_high) {
            glColor=TFT_YELLOW; // warning is YELLOW
          }
          if(sensSgv<cfg.red_low || sensSgv>cfg.red_high) {
            glColor=TFT_RED; // alert is RED
          }

          char glykStr[128];
          sprintf(glykStr, "Glyk: %4.1f %s", sensSgv, sensDir);
          Serial.println(glykStr);
          // M5.Lcd.println(glykStr);
          
          M5.Lcd.fillRect(0, 37, 160, 53, TFT_BLACK);
          M5.Lcd.setTextSize(2);
          // M5.Lcd.setTextDatum(TL_DATUM);
          M5.Lcd.setTextColor(glColor, TFT_BLACK);
          char sensSgvStr[30];
          int smaller_font = 0;
          if( cfg.show_mgdl ) {
            sprintf(sensSgvStr, "%3.0f", sensSgvMgDl);
          } else {
            sprintf(sensSgvStr, "%4.1f", sensSgv);
            if( sensSgvStr[0]!=' ' )
              smaller_font = 1;
          }
          // Serial.print("SGV string length = "); Serial.print(strlen(sensSgvStr));
          // Serial.print(", smaller_font = "); Serial.println(smaller_font);
          if( smaller_font ) {
            // M5.Lcd.setFreeFont(FSSB18);
            M5.Lcd.drawString(sensSgvStr, 0, 39, 4);
          } else {
            // M5.Lcd.setFreeFont(FSSB24);
            M5.Lcd.drawString(sensSgvStr, 0, 39, 4);
          }
          int tw=M5.Lcd.textWidth(sensSgvStr)*2;
          int th=M5.Lcd.fontHeight(4);
          Serial.print("textWidth="); Serial.println(tw);
          Serial.print("textHeight="); Serial.println(th);






//        ARROW SECTION




          int arrowAngle = 180;
          if(strcmp(sensDir,"DoubleDown")==0)
            arrowAngle = 90;
          else 
            if(strcmp(sensDir,"SingleDown")==0)
              arrowAngle = 75;
            else 
                if(strcmp(sensDir,"FortyFiveDown")==0)
                  arrowAngle = 45;
                else 
                    if(strcmp(sensDir,"Flat")==0)
                      arrowAngle = 0;
                    else 
                        if(strcmp(sensDir,"FortyFiveUp")==0)
                          arrowAngle = -45;
                        else 
                            if(strcmp(sensDir,"SingleUp")==0)
                              arrowAngle = -75;
                            else 
                                if(strcmp(sensDir,"DoubleUp")==0)
                                  arrowAngle = -90;
                                else 
                                    if(strcmp(sensDir,"NONE")==0)
                                      arrowAngle = 180;
                                    else 
                                        if(strcmp(sensDir,"NOT COMPUTABLE")==0)
                                          arrowAngle = 180;
          if(arrowAngle!=180)
            // drawArrow(0+tw+25, 0+40, 10, arrowAngle+85, 40, 40, glColor);
            drawArrow(112, 60, 20, arrowAngle+85, 20, 20, glColor);                          //    (X, Y, POINT CURVE, ANGLE ARROW, WIDTH, HEIGHT, COLOR)    

          /*
          // draw help lines
          for(int i=0; i<320; i+=40) {
            M5.Lcd.drawLine(i, 0, i, 240, TFT_DARKGREY);
          }
          for(int i=0; i<240; i+=30) {
            M5.Lcd.drawLine(0, i, 320, i, TFT_DARKGREY);
          }
          M5.Lcd.drawLine(0, 120, 320, 120, TFT_LIGHTGREY);
          M5.Lcd.drawLine(160, 0, 160, 240, TFT_LIGHTGREY);
          */






//    GRIND SECTION


          M5.Lcd.drawLine(0, 37, 200, 37, TFT_DARKGREY);          // HORIZONTAL LINE
          M5.Lcd.drawLine(85, 0, 85, 37, TFT_DARKGREY);           // VERTICAL LINE








          // drawMiniGraph();

          // calculate last alarm time difference
          int alarmDifSec=1000000;
          int snoozeDifSec=1000000;
          if(!getLocalTime(&timeinfo)){
            alarmDifSec=24*60*60; // too much
            snoozeDifSec=cfg.snooze_timeout*60; // timeout
          } else {
            alarmDifSec=difftime(mktime(&timeinfo), lastAlarmTime);
            snoozeDifSec=difftime(mktime(&timeinfo), lastSnoozeTime);
            if( snoozeDifSec>cfg.snooze_timeout*60 )
              snoozeDifSec=cfg.snooze_timeout*60; // timeout
          }
          Serial.print("Alarm time difference = "); Serial.print(alarmDifSec); Serial.println(" sec");
          Serial.print("Snooze time difference = "); Serial.print(snoozeDifSec); Serial.println(" sec");
          char tmpStr[10];
          if( snoozeDifSec<cfg.snooze_timeout*60 ) {
            sprintf(tmpStr, "%i", (cfg.snooze_timeout*60-snoozeDifSec+59)/60);
          } else {
            strcpy(tmpStr, "Snooze");
          }
          M5.Lcd.setTextSize(2);
          // M5.Lcd.setFreeFont(FSSB12);
          // Serial.print("sensSgv="); Serial.print(sensSgv); Serial.print(", cfg.snd_alarm="); Serial.println(cfg.snd_alarm); 
          if((sensSgv<=cfg.snd_alarm) && (sensSgv>=0.1)) {
            // red alarm state
            // M5.Lcd.fillRect(110, 220, 100, 20, TFT_RED);
            Serial.println("ALARM LOW");
            led_alert = 1;
            // M5.Lcd.fillRect(0, 220, 320, 20, TFT_RED);
            // M5.Lcd.setTextColor(TFT_BLACK, TFT_RED);
            // int stw=M5.Lcd.textWidth(tmpStr);
            // M5.Lcd.drawString(tmpStr, 159-stw/2, 220, 2);
            if( (alarmDifSec>cfg.alarm_repeat*60) && (snoozeDifSec==cfg.snooze_timeout*60) ) {
                sndAlarm();
                lastAlarmTime = mktime(&timeinfo);
            }
          } else {
            if((sensSgv<=cfg.snd_warning) && (sensSgv>=0.1)) {
              // yellow warning state
              // M5.Lcd.fillRect(110, 220, 100, 20, TFT_YELLOW);
              Serial.println("WARNING LOW");
              led_alert = 1;
              // M5.Lcd.fillRect(0, 220, 320, 20, TFT_YELLOW);
              // M5.Lcd.setTextColor(TFT_BLACK, TFT_YELLOW);
              // int stw=M5.Lcd.textWidth(tmpStr);
              // M5.Lcd.drawString(tmpStr, 159-stw/2, 220, 2);
              if( (alarmDifSec>cfg.alarm_repeat*60) && (snoozeDifSec==cfg.snooze_timeout*60) ) {
                sndWarning();
                lastAlarmTime = mktime(&timeinfo);
              }
            } else {
              if( sensSgv>=cfg.snd_alarm_high ) {
                // red alarm state
                // M5.Lcd.fillRect(110, 220, 100, 20, TFT_RED);
                Serial.println("ALARM HIGH");
                led_alert = 1;
                // M5.Lcd.fillRect(0, 220, 320, 20, TFT_RED);
                // M5.Lcd.setTextColor(TFT_BLACK, TFT_RED);
                // int stw=M5.Lcd.textWidth(tmpStr);
                // M5.Lcd.drawString(tmpStr, 159-stw/2, 220, 2);
                if( (alarmDifSec>cfg.alarm_repeat*60) && (snoozeDifSec==cfg.snooze_timeout*60) ) {
                    sndAlarm();
                    lastAlarmTime = mktime(&timeinfo);
                }
              } else {
                if( sensSgv>=cfg.snd_warning_high ) {
                  // yellow warning state
                  // M5.Lcd.fillRect(110, 220, 100, 20, TFT_YELLOW);
                  Serial.println("WARNING HIGH");
                  led_alert = 1;
                  // M5.Lcd.fillRect(0, 220, 320, 20, TFT_YELLOW);
                  // M5.Lcd.setTextColor(TFT_BLACK, TFT_YELLOW);
                  // int stw=M5.Lcd.textWidth(tmpStr);
                  // M5.Lcd.drawString(tmpStr, 159-stw/2, 220, 2);
                  if( (alarmDifSec>cfg.alarm_repeat*60) && (snoozeDifSec==cfg.snooze_timeout*60) ) {
                    sndWarning();
                    lastAlarmTime = mktime(&timeinfo);
                  }
                } else {
                  if( sensorDifMin>=cfg.snd_no_readings ) {
                    // yellow warning state
                    // M5.Lcd.fillRect(110, 220, 100, 20, TFT_YELLOW);
                    Serial.println("WARNING NO READINGS");
                    led_alert = 1;
                    // M5.Lcd.fillRect(0, 220, 320, 20, TFT_YELLOW);
                    // M5.Lcd.setTextColor(TFT_BLACK, TFT_YELLOW);
                    // int stw=M5.Lcd.textWidth(tmpStr);
                    // M5.Lcd.drawString(tmpStr, 159-stw/2, 220, 2);
                    if( (alarmDifSec>cfg.alarm_repeat*60) && (snoozeDifSec==cfg.snooze_timeout*60) ) {
                      sndWarning();
                      lastAlarmTime = mktime(&timeinfo);
                    }
                  } else {
                    // normal glycemia state
                    led_alert = 0;
                    // M5.Lcd.fillRect(0, 220, 320, 20, TFT_BLACK);
                    // M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
                    char devStr[64];
                    if( true ) {
                      strcpy(devStr, "    Bright  Snooze     Off");
                    } else {
                      strcpy(devStr, sensDev);
                      if(strcmp(devStr,"MIAOMIAO")==0) {
                        if(obj.containsKey("xDrip_raw")) {
                          strcpy(devStr,"xDrip MiaoMiao + Libre");
                        } else {
                          strcpy(devStr,"Spike MiaoMiao + Libre");
                        }
                      }
                      if(strcmp(devStr,"Tomato")==0)
                        strcat(devStr," MiaoMiao + Libre");
                    }
                    // M5.Lcd.drawString(devStr, 0, 220, 2);
                  }
                }
              }
            }
          }
        }
      } else {
        String errstr = String(" ");
        Serial.println(errstr);
        M5.Lcd.setCursor(0, 23);
        M5.Lcd.setTextSize(2);
        M5.Lcd.println(errstr);
        wasError = 1;
      }
    } else {
      String errstr = String(" ");
      Serial.println(errstr);
      M5.Lcd.setCursor(0, 23);
      M5.Lcd.setTextSize(2);
      M5.Lcd.println(errstr);
      wasError = 1;
    }
  
    http.end();
  }
}

// the loop routine runs over and over again forever
void loop(){
  delay(20);
  buttons_test();

  // update glycemia every 15s
  if(millis()-msCount>15000) {
    update_glycemia();
    msCount = millis();  
  } else {
    if(cfg.show_current_time) {
      // update current time on display
      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
      if(!getLocalTime(&localTimeInfo)) {
        // unknown time
        strcpy(localTimeStr,"??:??:??");
        lastSec = 61;
      } else {
        if(getLocalTime(&localTimeInfo)) {
          sprintf(localTimeStr, "%02d:%02d:%02d  %d.%d.  ", localTimeInfo.tm_hour, localTimeInfo.tm_min, localTimeInfo.tm_sec, localTimeInfo.tm_mday, localTimeInfo.tm_mon+1);  
        } else {
          strcpy(localTimeStr, "??:??:??");
          lastSec = 61;
        }
      }
      if(lastSec!=localTimeInfo.tm_sec) {
        lastSec=localTimeInfo.tm_sec;
        // M5.Lcd.drawString(localTimeStr, 0, 0, 2);
      }
    }
    // Serial.print("led_alert="); Serial.print(led_alert); Serial.print(", millis()="); Serial.print(millis()); Serial.print(", msCountAlert="); Serial.println(msCountAlert);
    if( millis()-msCountAlert>500 ) {
      if(led_alert) {
        if(digitalRead(M5_LED)==LOW) {
          digitalWrite(M5_LED, HIGH);  
        } else {
          digitalWrite(M5_LED, LOW);  
        }
        // Serial.println(digitalRead(M5_LED));
      } else {
        digitalWrite(M5_LED, HIGH);
      }
      msCountAlert = millis();
    }
  }

  yield();
}
