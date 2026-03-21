#include "config.h"
#include "saveFile.h"
#include "globals.h"

void saveIdToFile(uint16_t id) {
    char buffer[10]; itoa(id, buffer, 10);
    writeFile(LITTLEFS, "/id.txt", buffer);
}

void saveStepsToFile(uint32_t step_count) {
    char buffer[10]; itoa(step_count, buffer, 10);
    writeFile(LITTLEFS, "/steps.txt", buffer);
}

void saveDistanceToFile(float distance) {
    char buffer[16]; dtostrf(distance, 6, 3, buffer);
    writeFile(LITTLEFS, "/distance.txt", buffer);
}

void saveDateTimeToFile(const String &dur, const String &startTime, const String &startDate) {
    String content = dur + ";" + startDate + ";" + startTime;
    writeFile(LITTLEFS, "/datetime.txt", content.c_str());
}

void deleteSession() {
    deleteFile(LITTLEFS, "/id.txt");
    deleteFile(LITTLEFS, "/distance.txt");
    deleteFile(LITTLEFS, "/steps.txt");
    deleteFile(LITTLEFS, "/datetime.txt");
    deleteFile(LITTLEFS, "/coord.txt");
}