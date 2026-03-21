#include "config.h"
#include "gps.h"
#include "globals.h"

// GPS
void logGps() {
    watch->gpsHandler();
    if (!gps) {
        return;
    }
    if (!gps->location.isValid() || !gps->location.isUpdated()) {
        return;
    }
    if (millis() - lastGpsSave < 3000) {
        return;
    }
    lastGpsSave = millis();
    if (gpsPointCount < MAX_GPS_POINTS) {
        gpsPoints[gpsPointCount++] = {gps->location.lat(), gps->location.lng()};
    }
}

void saveGpsPointsToFile() {
    File file = LITTLEFS.open("/coord.txt", FILE_WRITE);
    if (!file) { 
        Serial.println("Failed to open /coord.txt"); 
        return; 
    }
    for (int i = 0; i < gpsPointCount; i++) {
        file.print(gpsPoints[i].lat, 6); file.print(";");
        file.print(gpsPoints[i].lon, 6); file.print(";");
    }
    file.close();
}
