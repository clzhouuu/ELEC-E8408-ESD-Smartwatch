#include "config.h"
#include "bluetooth.h"
#include "globals.h"
#include "screens.h"
#include "saveFile.h"

// send files
void sendDataBT(fs::FS &fs, const char *path) {
    fs::File file = fs.open(path);
    if (!file || file.isDirectory()) {
        Serial.println("failed to open file for reading");
        return;
    }
    while (file.available()) SerialBT.write(file.read());
    file.close();
}

void sendSessionBT() {
    lv_scr_load(scr_sync);
    lv_task_handler();
    sendDataBT(LITTLEFS, "/id.txt"); 
    SerialBT.write(';');
    sendDataBT(LITTLEFS, "/steps.txt"); 
    SerialBT.write(';');
    sendDataBT(LITTLEFS, "/distance.txt"); 
    SerialBT.write(';');
    sendDataBT(LITTLEFS, "/datetime.txt");
    SerialBT.write(';');
    sendDataBT(LITTLEFS, "/coord.txt");
    SerialBT.write('\n');
}



// BTsync 
void BTsync() {
    if (!SerialBT.available()) {
        return;
    }

    char incomingChar = SerialBT.read();
    if (incomingChar == 'c' && sessionStored && !sessionSent) 
    {
        sendSessionBT();
        sessionSent = true;
    }

    if (sessionSent && sessionStored) 
    {
        unsigned long syncStart = millis();

        while (millis() - syncStart < 2000) 
        {
            if (SerialBT.available()) 
                incomingChar = SerialBT.read();
            if (incomingChar == 'r') 
            {
                Serial.println("Got an R");
                deleteSession();
                sessionId++;
                sessionStored = false;
                sessionSent = false;

                lv_obj_t *scr_saved = lv_obj_create(NULL, NULL);
                setBackground(scr_saved);
                addHuippuLogo(scr_saved);
                makeLabel(scr_saved, "SAVED", 76, 105, LV_COLOR_BLACK, FONT_HUGE);
                lv_scr_load(scr_saved);
                lv_task_handler();
                delay(3000);
                lv_obj_del(scr_saved);
                return;
            }
        }

        sessionSent = false;
        Serial.println("Sync timeout: session kept on device");

        lv_obj_t *scr_savedFail = lv_obj_create(NULL, NULL);
        setBackground(scr_savedFail);
        addHuippuLogo(scr_savedFail);
        lv_obj_t *fail_box = makeCard(scr_savedFail, 15, 45, 210, 150, 16);
        makeLabel(fail_box, "SYNC FAILED", 61, 20, LV_COLOR_BLACK, FONT_MEDIUM);
        makeLabel(fail_box, "Session saved", 44, 60, LV_COLOR_BLACK, FONT_MEDIUM);
        makeLabel(fail_box, "on device", 66, 95, LV_COLOR_BLACK, FONT_MEDIUM);
        lv_scr_load(scr_savedFail);
        lv_task_handler();
        delay(4000);
        lv_obj_del(scr_savedFail);
    }
}

void receiveBTConfig() {
    if (!SerialBT.available()) {
        return;
    }

    String incoming = SerialBT.readStringUntil('\n');
    incoming.trim();
    
    int first  = incoming.indexOf(';');
    int second = incoming.indexOf(';', first + 1);
    
    if (first != -1 && second != -1) {
        weight = incoming.substring(first + 1, second).toFloat();
        height = incoming.substring(second + 1).toFloat();
        stride = 0.43f * height;
        Serial.printf("Config received: weight=%.1f height=%.1f\n", weight, height);
        SerialBT.write('r'); 
    }
}