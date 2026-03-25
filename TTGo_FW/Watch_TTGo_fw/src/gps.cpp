#include "config.h"
#include "gps.h"
#include "globals.h"

int lastSavedIndex = 0;

// GPS
void logGps() {
    watch->gpsHandler();
    if (!gps) return;

    if (!gps->location.isValid() || 
        !gps->location.isUpdated() ||
        !gps->altitude.isValid()) {
        return;
    }

    if (millis() - lastGpsSave < 3000) return;

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
        file.print(";\n");
    }

    lastSavedIndex = gpsPointCount;

    file.close();
}