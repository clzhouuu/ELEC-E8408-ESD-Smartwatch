#include "config.h"
#include "gps.h"
#include "globals.h"

int lastSavedIndex = 0;

// GPS
void logGps() {
    int last = 0;

    watch->gpsHandler();

    if (!gps) { Serial.println("GPS: null ptr"); return; }

    if (!gps->location.isValid()) { 
        Serial.print("Chars: ");
        Serial.println(gps->charsProcessed());

        Serial.print("Satellite value: ");
        Serial.println(gps->satellites.value());

        Serial.print("HDOP value: ");
        Serial.println(gps->hdop.value());
        
        Serial.println("GPS: location invalid"); 

        return; }
    if (!gps->altitude.isValid()) { Serial.println("GPS: altitude invalid"); return; }



    if (!gps) {
        return;
    }

    if (!gps->location.isValid() ||
        !gps->altitude.isValid()) {
        return;
    }

    if (millis() - lastGpsSave < 10000) return; 

    lastGpsSave = millis();

    if (gpsPointCount < MAX_GPS_POINTS) {
        gpsPoints[gpsPointCount++] = {
            gps->location.lat(), gps->location.lng(), gps->altitude.meters()
        };
    }
}

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
        file.print(",");
        file.print(gpsPoints[i].alt, 2); 
        file.print(";");
    }

    lastSavedIndex = gpsPointCount;

    file.close();
}

void updateTime() {
    if (!gps) 
        return;

    if (gps->date.isValid() && gps->time.isValid()) {
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