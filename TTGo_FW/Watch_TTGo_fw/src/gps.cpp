#include "config.h"
#include "gps.h"
#include "globals.h"

int lastSavedIndex = 0;

// GPS
void logGps() {
    // error messages for debugging     
    if (!gps) { 
        Serial.println("GPS: null ptr"); 
        return; 
    }
    if (!gps->satellites.isValid() || gps->satellites.value() < 3) {
        Serial.println("GPS: not enough satellites");         
        Serial.print("Satellite value: ");
        Serial.println(gps->satellites.value());
        return;
    }
    if (!gps->location.isValid()) { 
        Serial.print("Chars: ");
        Serial.println(gps->charsProcessed());


        Serial.print("HDOP value: ");
        Serial.println(gps->hdop.value());
        
        Serial.println("GPS: location invalid"); 

        return; }

    // if gps has no saves in the last 10 seconds, dont save gps point
    if (millis() - lastGpsSave < 10000) {
        return; 
    }
    else if (gpsPointCount < MAX_GPS_POINTS) {
        gpsPoints[gpsPointCount++] = {
            gps->location.lat(), gps->location.lng()
        };
        lastGpsSave = millis();
    }
}

// save gps points to file
void saveGpsPointsToFile() {
    File file = LITTLEFS.open("/coord.txt", FILE_APPEND);
    if (!file) { 
        Serial.println("Failed to open /coord.txt"); 
        return; 
    }

    for (int i = lastSavedIndex; i < gpsPointCount; i++) {
        file.print(gpsPoints[i].lat, 6); 
        file.print(",");
        file.print(gpsPoints[i].lon, 6); 
        file.print(";");
    }

    lastSavedIndex = gpsPointCount;

    file.close();
}

// updates time based on GPS data if valid, otherwise does nothing
void updateTime() {
    if (!gps) {
        Serial.println("GPS: null ptr");      
        return;
    }

    if (!gps->satellites.isValid() || gps->satellites.value() < 3) {
        Serial.println("GPS: not enough satellites");         
        Serial.print("Satellite value: ");
        Serial.println(gps->satellites.value());
        return;
    }

    if (gps->date.isValid() && gps->time.isValid() && gps->satellites.isValid() && gps->satellites.value() >= 3) {
        int year   = gps->date.year();
        int month  = gps->date.month();
        int day    = gps->date.day();
        int hour   = gps->time.hour(); 
        int minute = gps->time.minute();
        int second = gps->time.second();

        if (hour >= 24) {
            hour -= 24;
        }

        rtc->setDateTime(year, month, day, hour, minute, second);
        
        Serial.printf("RTC synced from GPS: %04d-%02d-%02d %02d:%02d:%02d\n",
                    year, month, day, hour, minute, second);
    }
}       
