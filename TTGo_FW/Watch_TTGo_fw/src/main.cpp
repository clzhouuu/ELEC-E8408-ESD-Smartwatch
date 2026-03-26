#include "config.h"
#include "image.c"
#include "globals.h"
#include "bluetooth.h"
#include "power.h"
#include "gps.h"
#include "saveFile.h"
#include "screens.h"

using fs::File;

// Check if Bluetooth configs are enabled
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// global definitions
TTGOClass *watch;
BMA *sensor;
PCF8563_Class *rtc;
TinyGPSPlus *gps;
BluetoothSerial SerialBT;

uint32_t sessionId = 0; 
#define SLEEP_TIMEOUT_MS 30000

// input variables
float height = 170.0f / 100.0f; // average weight and height
float weight = 75.0f;

// distance calculation variables
uint32_t currentSteps = 0;
uint32_t lastStep = 0;
float distance_m = 0.0f;
float stride = 0.43f * height;

// time variables
String sessionStartDate = "";
String sessionStartTime = "";
String sessionEndTime = "";
unsigned long sessionStartMs = 0;
unsigned long sessionDurationMs = 0;
String durationStr = "";

// calorie estimation variables
const float MET = 6;
uint32_t caloriesBurned = 0;

// state machine
volatile uint8_t state = 1;

// interrupts
volatile bool irqBMA = false;
volatile bool irqButton = false;
volatile bool irqButtonLong = false;

// battery
int batteryPercent = 0;
bool s_isCharging = false;
unsigned long batteryTimer = 0;

// sleep
bool screenAsleep = false;
unsigned long lastActivity = 0;

// session sync
bool sessionStored = LittleFS.exists("/id.txt") &&
                LittleFS.exists("/steps.txt") &&
                LittleFS.exists("/distance.txt") &&
                LittleFS.exists("/datetime.txt");
bool sessionSent = false;

GpsPoint gpsPoints[MAX_GPS_POINTS];
int gpsPointCount = 0;
unsigned long lastGpsSave = 0;
unsigned long lastGpsFileSave = 0;

int monthStrToNumber(String month) {
    if (month == "Jan") return 1;
    if (month == "Feb") return 2;
    if (month == "Mar") return 3;
    if (month == "Apr") return 4;
    if (month == "May") return 5;
    if (month == "Jun") return 6;
    if (month == "Jul") return 7;
    if (month == "Aug") return 8;
    if (month == "Sep") return 9;
    if (month == "Oct") return 10;
    if (month == "Nov") return 11;
    if (month == "Dec") return 12;
    return 1;
}


// touchscreen
void requestTouchWake() {
    lastActivity = millis();
    if (screenAsleep) {
        screenWake();
    }
}

void startHikeBtnEvent(lv_obj_t *obj, lv_event_t event) {
    if (event != LV_EVENT_CLICKED) return;

    if (screenAsleep) {
        screenWake();
        return;
    }

    static unsigned long lastPress = 0;
    if (millis() - lastPress < 1000) return;
    lastPress = millis();

    if (state == 1) {
        state = 2;
    }
}

void endHikeBtnEvent(lv_obj_t *obj, lv_event_t event) {
    if (event != LV_EVENT_CLICKED) return;

    if (screenAsleep) {
        requestTouchWake();
        return;
    }

    static unsigned long lastPress = 0;
    if (millis() - lastPress < 1000) return;
    lastPress = millis();

    if (state == 3) {
        sessionEndTime = String(rtc->formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S));
        sessionDurationMs = millis() - sessionStartMs;
        state = 4;
    }
}


void wakeTouchEvent(lv_obj_t *obj, lv_event_t event) {
    if (event == LV_EVENT_PRESSED || event == LV_EVENT_PRESSING || event == LV_EVENT_CLICKED) {
        requestTouchWake();
    }
}

// setup
void setup() {
    Serial.begin(115200);
    watch = TTGOClass::getWatch();
    watch->begin();
    watch->lvgl_begin();
    watch->openBL();

    Serial.print("__DATE__ = ");
    Serial.println(__DATE__);
    Serial.print("__TIME__ = ");
    Serial.println(__TIME__);

    sensor = watch->bma;
    rtc = watch->rtc;
    rtc->check();

    watch->trunOnGPS();
    watch->gps_begin();
    gps = watch->gps;

    initHikeWatch();
    buildIdleScreen();
    buildHikeScreen();
    buildSavingScreen();
    buildSyncScreen();

    deleteSession();

    sessionStored = LittleFS.exists("/id.txt") &&
            LittleFS.exists("/steps.txt") &&
            LittleFS.exists("/distance.txt") &&
            LittleFS.exists("/datetime.txt");

    rtc->setDateTime(
        String(__DATE__).substring(7, 11).toInt(),   
        monthStrToNumber(String(__DATE__).substring(0, 3)),
        String(__DATE__).substring(4, 6).toInt(),   
        String(__TIME__).substring(0, 2).toInt(),   
        String(__TIME__).substring(3, 5).toInt(),
        String(__TIME__).substring(6, 8).toInt()   
    );

    if (!gps) { Serial.println("GPS: null ptr"); return; }
    if (!gps->location.isValid()) { Serial.println("GPS: location invalid"); return; }
    if (!gps->altitude.isValid()) { Serial.println("GPS: altitude invalid"); return; }


    sessionId = 1;
    state = 1;
    lastActivity = millis(); 
    SerialBT.begin("Hiking Watch");
    Serial.println("Bluetooth started");
}


// loop
void loop() {
    lv_task_handler();

    switch (state) {
    case 1:
    {
        Serial.print("__DATE__ = ");
        Serial.println(__DATE__);
        Serial.print("__TIME__ = ");
        Serial.println(__TIME__);
        Serial.print("RTC after set = ");
        Serial.println(rtc->formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S));
        /* Initial stage */
        lv_scr_load(scr_idle);
        lv_label_set_text(lbl_idle_bigtime, rtc->formatDateTime(PCF_TIMEFORMAT_HM));
        lv_label_set_text(lbl_idle_date, rtc->formatDateTime(PCF_TIMEFORMAT_DD_MM_YYYY));
        lv_label_set_text(lbl_idle_batt_icon, battIcon());
        lv_label_set_text(lbl_idle_batt_pct, battPctStr().c_str());

        if (sessionStored) {
            lv_label_set_text(lbl_idle_btstate, "Session stored");
        } else {
            lv_label_set_text(lbl_idle_btstate, "No session data");
        }

        lv_task_handler();
        
        
        //Bluetooth discovery
        while (1) 
        {
            lv_task_handler();

            if (!screenAsleep && millis() - lastActivity > SLEEP_TIMEOUT_MS) {
                screenSleep();
            }

            static unsigned long lastTick = 0;
            if (millis() - lastTick > 1000) {
                lastTick = millis();
                readBattery();
                lv_label_set_text(lbl_idle_bigtime, rtc->formatDateTime(PCF_TIMEFORMAT_HM));
                lv_label_set_text(lbl_idle_date, rtc->formatDateTime(PCF_TIMEFORMAT_DD_MM_YYYY));
                lv_label_set_text(lbl_idle_batt_icon, battIcon());
                lv_label_set_text(lbl_idle_batt_pct, battPctStr().c_str());
                lv_label_set_text(lbl_idle_charge_icon, chargeIcon());

                setBtIconColor(lbl_idle_bt_icon);
                setGpsIconColor(lbl_idle_gps_icon);
            }

            // while (Serial1.available()) {
            //     char c = Serial1.read();
            //     Serial.write(c);
            // }



            /* Bluetooth sync */
            if (SerialBT.available()) {
                int msg = SerialBT.peek();
                if (msg == 'c') {
                    BTsync();
                } else {
                    receiveBTonfig();
                }
            }

            static bool rtcSyncedFromGps = false;
            static unsigned long lastRtcSync = 0;

            if (gps &&
                gps->date.isValid() &&
                gps->time.isValid() &&
                gps->date.isUpdated() &&
                gps->time.isUpdated() &&
                gps->location.isValid() &&
                gps->location.age() < 10000)
            {
                int year   = gps->date.year();
                int month  = gps->date.month();
                int day    = gps->date.day();
                int hour   = gps->time.hour() + 1; 
                int minute = gps->time.minute();
                int second = gps->time.second();

                if (hour >= 24) hour -= 24;

                if (!rtcSyncedFromGps || millis() - lastRtcSync > 60000) {
                    rtc->setDateTime(year, month, day, hour, minute, second);
                    lastRtcSync = millis();
                    rtcSyncedFromGps = true;

                    Serial.printf("RTC synced from GPS: %04d-%02d-%02d %02d:%02d:%02d\n",
                                year, month, day, hour, minute, second);
                }
            }
            if (state == 2) {
                break;
            }
        }
        break;
    }

    case 2:
    {
        /* Hiking session initialisation */

        // Checking for previous session and resyncing

        lv_obj_t *scr_start = lv_obj_create(NULL, NULL);
        setBackground(scr_start);
        makeLabel(scr_start, "HUIPPU", 8, 8, LV_COLOR_BLACK, FONT_SMALL);
        makeLabel(scr_start, "Starting hike", 65, 108, LV_COLOR_BLACK, FONT_MEDIUM);
        lv_scr_load(scr_start);
        lv_task_handler();

        unsigned long startTime = millis();
        while (millis() - startTime < 2000) {
            lv_task_handler();
        }

 
        if (sessionStored) {
            lv_obj_t *scr_warn = lv_obj_create(NULL, NULL);
            setBackground(scr_warn);
            addHuippuLogo(scr_warn);
            lv_obj_t *warn_box = makeCard(scr_warn, 15, 40, 210, 160, 16);
            makeLabel(warn_box, "CAUTION!", 30, 12, LV_COLOR_BLACK, FONT_HUGE);
            makeLabel(warn_box, "UNSYNCED SESSION", 20, 68, LV_COLOR_BLACK, FONT_MEDIUM);
            makeLabel(warn_box, "WILL BE", 72, 90, LV_COLOR_BLACK, FONT_MEDIUM);
            makeLabel(warn_box, "OVERWRITTEN", 40, 112, LV_COLOR_BLACK, FONT_MEDIUM);
            lv_scr_load(scr_warn);

            unsigned long warnStart = millis();
            while (millis() - warnStart < 2000) {
                lv_task_handler();
            }

            lv_obj_del(scr_warn);
            deleteSession();
        }

        lv_obj_del(scr_start);
        gpsPointCount = 0;
        lastGpsSave = 0;
        lastGpsFileSave = 0;

        state = 3;

        break;
    }

    case 3:
    {
         /* Hiking session ongoing */

        lv_scr_load(scr_hike);

        readBattery();
        lv_label_set_text(lbl_hike_batt_icon, battIcon());
        lv_label_set_text(lbl_hike_batt_pct, battPctStr().c_str());
        lv_label_set_text(lbl_hike_charge_icon, chargeIcon());

        // reset display values
        lv_label_set_text(lbl_steps_val, "0");
        lv_label_set_text(lbl_dist_val, "0.0km");
        lv_label_set_text(lbl_kcal_val, "0kcal");
        lv_label_set_text(lbl_dur_val, "00:00:00");
        lv_label_set_text(lbl_hike_low_batt, "");

        sessionStartDate = String(rtc->formatDateTime(PCF_TIMEFORMAT_DD_MM_YYYY));
        sessionStartTime = String(rtc->formatDateTime(PCF_TIMEFORMAT_HMS));
        sessionStartMs = millis();
        sessionDurationMs = 0;

        // reset counters
        sensor->resetStepCounter();
        lastStep = 0;
        distance_m = 0.0f;
        currentSteps = 0;
        caloriesBurned = 0;


        lastActivity = millis(); 
        lv_task_handler();

        // loop
        while (state == 3) {
            lv_task_handler();

            if (sessionDurationMs > 3UL * 3600000UL) {
                lastActivity = millis();
                sessionEndTime = String(rtc->formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S));
                sessionDurationMs = millis() - sessionStartMs;
                state = 4;
            }

            if (!screenAsleep && millis() - lastActivity > SLEEP_TIMEOUT_MS) {
                screenSleep();
            }

            lv_label_set_text(lbl_hike_toptime, rtc->formatDateTime(PCF_TIMEFORMAT_HM));

            // GPS
            logGps();
            if (millis() - lastGpsFileSave > 15000) {
                lastGpsFileSave = millis();
                saveGpsPointsToFile();
            }

            // duration
            static unsigned long lastDur = 0;
            if (millis() - lastDur > 1000) {
                lastDur = millis();
                sessionDurationMs = millis() - sessionStartMs;
                unsigned long s = sessionDurationMs / 1000;
                durationStr = twoDigits(s / 3600) + ":" + twoDigits((s % 3600) / 60) + ":" + twoDigits(s % 60);
                lv_label_set_text(lbl_dur_val, durationStr.c_str());
            }

            // BMA interrupt
            if (irqBMA) {
                irqBMA = false;
                sensor->readInterrupt();
                if (sensor->isStepCounter()) {
                    currentSteps = sensor->getCounter();
                    uint32_t delta = currentSteps - lastStep;
                    lastStep = currentSteps;
                    distance_m += delta * stride;
                    caloriesBurned = weight  * MET * ( distance_m / 1000.0f / 5.0f);

                    lv_label_set_text_fmt(lbl_steps_val, "%u", currentSteps);
                    char dist_buf[16];
                    dtostrf(distance_m / 1000.0f, 4, 2, dist_buf);
                    strcat(dist_buf, "km");
                    lv_label_set_text(lbl_dist_val, dist_buf);
                    lv_label_set_text_fmt(lbl_kcal_val, "%ukcal", caloriesBurned);
                }
            }

            setGpsIconColor(lbl_hike_gps);

            // battery 
            if (millis() - batteryTimer > 5000) {
                batteryTimer = millis();
                readBattery();
                lv_label_set_text(lbl_hike_batt_icon, battIcon());
                lv_label_set_text(lbl_hike_batt_pct, battPctStr().c_str());
                lv_label_set_text(lbl_hike_low_batt, batteryPercent < 20 ? "LOW BATTERY!" : "");
                setBtIconColor(lbl_hike_bt_icon);
                lv_label_set_text(lbl_hike_charge_icon, chargeIcon());

            }


        }

        break;
    }

    case 4:
    {
        lv_scr_load(scr_saving);
        lv_task_handler();

        // save hiking session data
        saveGpsPointsToFile();
        saveIdToFile(sessionId);
        saveDistanceToFile(distance_m / 1000.0f);
        saveStepsToFile(sensor->getCounter());
        saveKcalToFile(caloriesBurned);
        saveDateTimeToFile(durationStr, sessionStartTime, sessionStartDate);

        sessionStored = true;
        sessionSent = false;

        // bt sync
        if (SerialBT.hasClient()) {
            lv_scr_load(scr_sync); 
            lv_task_handler();
        }

        BTsync();
 
        if (sessionStored) 
        {
            Serial.println("BT not connected or sync failed: Session saved locally");
 
            lv_obj_t *scr_nobt = lv_obj_create(NULL, NULL);
            setBackground(scr_nobt);
            addHuippuLogo(scr_nobt);
            lv_obj_t *nobt_box = makeCard(scr_nobt, 15, 45, 210, 150, 16);
            makeLabel(nobt_box, "SAVED!", 48, 15, LV_COLOR_BLACK, FONT_HUGE);
            makeLabel(nobt_box, "No BT connection", 35, 75, LV_COLOR_BLACK, FONT_MEDIUM);
            makeLabel(nobt_box, "to sync session", 50, 98, LV_COLOR_BLACK, FONT_MEDIUM);
            lv_scr_load(scr_nobt);
            lv_task_handler();

            unsigned long nobtTime = millis();
            while (millis() - nobtTime < 2000) {
                lv_task_handler();
            }

            lv_obj_del(scr_nobt);
        } 

        lastActivity = millis(); 
        state = 1;

        break;
    }

    default:
        ESP.restart();
        break;
    }
}