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
    sendDataBT(LITTLEFS, "/kcal.txt");
    SerialBT.write(';');
    sendDataBT(LITTLEFS, "/datetime.txt");
    SerialBT.write(';');
    sendDataBT(LITTLEFS, "/coord.txt");
    SerialBT.write('\n');

    Serial.println("/id.txt");
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

        while (millis() - syncStart < 5000) 
        {
            if (SerialBT.available()) 
                incomingChar = SerialBT.read();
            if (incomingChar == 'r') 
            {
                Serial.println("Got an R");
                sessionId++;
                sessionStored = false;
                sessionSent = false;

                lv_obj_t *scr_saved = lv_obj_create(NULL, NULL);
                setBackground(scr_saved);
                addHuippuLogo(scr_saved);
                makeLabel(scr_saved, "SENT", 78, 105, LV_COLOR_BLACK, FONT_MEDIUM);
                lv_scr_load(scr_saved);
                lv_task_handler();

                unsigned long savedStart = millis();
                while (millis() - savedStart < 2000) {
                    lv_task_handler();
                }

                lv_scr_load(scr_idle);
                lv_obj_del(scr_saved);
                
                deleteSession();
                return;
            }
        }

        sessionSent = false;
        Serial.println("Sync timeout: session kept on device");

        lv_obj_t *scr_savedFail = lv_obj_create(NULL, NULL);
        setBackground(scr_savedFail);
        addHuippuLogo(scr_savedFail);
        lv_obj_t *fail_box = makeCard(scr_savedFail, 15, 45, 210, 150, 16);
        makeLabel(fail_box, "SYNC FAILED", 62, 20, LV_COLOR_BLACK, FONT_MEDIUM);
        makeLabel(fail_box, "Session saved", 44, 60, LV_COLOR_BLACK, FONT_MEDIUM);
        makeLabel(fail_box, "on device", 66, 95, LV_COLOR_BLACK, FONT_MEDIUM);
        lv_scr_load(scr_savedFail);
        lv_task_handler();

        unsigned long failedStart = millis();
        while (millis() - failedStart < 2000) {
            lv_task_handler();
        }

        lv_obj_del(scr_savedFail);
    }
}

// receive BT config
void receiveBTConfig() {
    if (!SerialBT.available()) {
        return;
    }

    String incoming = SerialBT.readStringUntil('\n');
    incoming.trim();
    
    if (incoming.startsWith("CONFIG")) {
        int first  = incoming.indexOf(',');
        int second = incoming.indexOf(',', first + 1);
        
        if (first != -1 && second != -1) {
            weight = incoming.substring(first + 1, second).toFloat();
            height = incoming.substring(second + 1).toFloat() / 100.0f;
            stride = 0.43f * height;
            Serial.printf("Config received: weight=%.1f height=%.1f\n", weight, height);
            SerialBT.write('r'); 
    }
    }


}