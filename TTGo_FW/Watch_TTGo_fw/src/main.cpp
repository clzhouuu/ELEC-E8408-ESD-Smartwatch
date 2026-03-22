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
const float KCAL_PER_STEP = 0.04;

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
bool sessionStored = false;
bool sessionSent = false;

GpsPoint gpsPoints[MAX_GPS_POINTS];
int gpsPointCount = 0;
unsigned long lastGpsSave = 0;
unsigned long lastGpsFileSave = 0;
bool rtcSynced = false;

// setup
void setup() {
    Serial.begin(115200);
    watch = TTGOClass::getWatch();
    watch->begin();
    watch->lvgl_begin();
    watch->openBL();

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
        /* Initial stage */
        lv_scr_load(scr_idle);
        lv_label_set_text(lbl_idle_bigtime, rtc->formatDateTime(PCF_TIMEFORMAT_HM));
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
            if (gps) {
                watch->gpsHandler();
            }

            // if (!screenAsleep && millis() - lastActivity > SLEEP_TIMEOUT_MS) {
            //     screenSleep();
            // }

            static unsigned long lastTick = 0;
            if (millis() - lastTick > 1000) {
                lastTick = millis();
                readBattery();
                lv_label_set_text(lbl_idle_bigtime, rtc->formatDateTime(PCF_TIMEFORMAT_HM));
                lv_label_set_text(lbl_idle_batt_icon, battIcon());
                lv_label_set_text(lbl_idle_batt_pct, battPctStr().c_str());
                lv_label_set_text(lbl_idle_charge_icon, chargeIcon());

                setBtIconColor(lbl_idle_bt_icon);
                setGpsIconColor(lbl_idle_gps_icon);
            }

            /* Bluetooth sync */
            if (SerialBT.available()) {
                int msg = SerialBT.peek();
                if (msg == 'c') {
                    BTsync();
                } else {
                    receiveBTConfig();
                }
            }

            /*      IRQ     */
            if (irqButton) {
                irqButton = false;
                watch->power->readIRQ();

                lastActivity = millis();
                if (state == 1) {
                    state = 2;
                }

                // if (watch->power->isPEKLongtPressIRQ()) 
                // {
                //     watch->power->clearIRQ();
                //     shutDown();
                // } 
                // else if (watch->power->isPEKShortPressIRQ()) 
                // {
                //     if (screenAsleep) 
                //     {
                //         screenWake();
                //     } 
                //     else 
                //     {
                //         lastActivity = millis();
                        // if (state == 1) {
                        //     state = 2;
                        // }
                //      }
                // }

                watch->power->clearIRQ();
            }

            if (!rtcSynced && gps && gps->time.isValid() && gps->date.isValid()) {
                int hour = gps->time.hour() + 2;
                if (hour >= 24) {
                    hour -= 24;
                }

                rtc->setDateTime(
                    gps->date.year(),
                    gps->date.month(),
                    gps->date.day(),
                    hour,
                    gps->time.minute(),
                    gps->time.second()
                );
                rtcSynced = true;
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
        if (sessionStored) {
            if (SerialBT.hasClient()) {
                Serial.println("Retrying sync");
                lv_scr_load(scr_sync);
                lv_task_handler();
                BTsync();
            }
 
            if (sessionStored) {
                lv_obj_t *scr_warn = lv_obj_create(NULL, NULL);
                setBackground(scr_warn);
                addHuippuLogo(scr_warn);
                lv_obj_t *warn_box = makeCard(scr_warn, 15, 40, 210, 160, 16);
                makeLabel(warn_box, "CAUTION!", 28, 12, LV_COLOR_BLACK, FONT_HUGE);
                makeLabel(warn_box, "UNSYNCED SESSION", 20, 68, LV_COLOR_BLACK, FONT_MEDIUM);
                makeLabel(warn_box, "WILL BE", 72, 90, LV_COLOR_BLACK, FONT_MEDIUM);
                makeLabel(warn_box, "OVERWRITTEN", 40, 112, LV_COLOR_BLACK, FONT_MEDIUM);
                lv_scr_load(scr_warn);
                lv_task_handler();
                delay(5000);
                lv_obj_del(scr_warn);
            }
        }

        deleteFile(LITTLEFS, "/coord.txt");
        gpsPointCount = 0;
        lastGpsSave = 0;
        lastGpsFileSave = 0;

        state = 3;

        break;
    }

    case 3:
    {
         /* Hiking session ongoing */
        sessionStartDate = String(rtc->formatDateTime(PCF_TIMEFORMAT_DD_MM_YYYY));
        sessionStartTime = String(rtc->formatDateTime(PCF_TIMEFORMAT_HMS));
        sessionStartMs = millis();

        lv_obj_t *scr_start = lv_obj_create(NULL, NULL);
        setBackground(scr_start);
        makeLabel(scr_start, "HUIPPU", 8, 8, LV_COLOR_BLACK, FONT_SMALL);
        makeLabel(scr_start, "Starting hike", 65, 108, LV_COLOR_BLACK, FONT_MEDIUM);
        lv_scr_load(scr_start);
        lv_task_handler();
        delay(2000);
        lv_obj_del(scr_start);

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

        // reset counters
        sensor->resetStepCounter();
        lastStep = 0;
        distance_m = 0.0f;
        currentSteps = 0;
        caloriesBurned = 0;

        // reset irq
        irqButton = false;
        watch->power->readIRQ();
        watch->power->clearIRQ();

        lastActivity = millis(); 
        lv_task_handler();

        // loop
        while (state == 3) {
            lv_task_handler();

            // if (!screenAsleep && millis() - lastActivity > SLEEP_TIMEOUT_MS) {
            //     screenSleep();
            // }

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
            if (irqBMA && sessionDurationMs <= 36000000 * 3) {
                irqBMA = false;
                sensor->readInterrupt();
                if (sensor->isStepCounter()) {
                    currentSteps = sensor->getCounter();
                    uint32_t delta = currentSteps - lastStep;
                    lastStep = currentSteps;
                    distance_m += delta * stride;
                    caloriesBurned = distance_m * weight * (MET / 1.5) / 1000;

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

            /*      IRQ     */
            if (irqButton) {
                irqButton = false;
                watch->power->readIRQ();

                lastActivity = millis();
                sessionEndTime = String(rtc->formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S));
                sessionDurationMs = millis() - sessionStartMs;
                state = 4;

                // if (watch->power->isPEKLongtPressIRQ()) 
                // {
                //     watch->power->clearIRQ();
                //     shutDown();
                // } 
                // else if (watch->power->isPEKShortPressIRQ()) 
                // {
                //     if (screenAsleep) 
                //     {
                //         screenWake();
                //     }
                //     else 
                //     {
                //         lastActivity = millis();
                //         sessionEndTime = String(rtc->formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S));
                //         sessionDurationMs = millis() - sessionStartMs;
                //         state = 4;
                //     }
                // }
                // watch->power->clearIRQ();
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
        saveStepsToFile(sensor->getCounter());
        saveDistanceToFile(distance_m / 1000.0f);
        saveDateTimeToFile(durationStr, sessionStartTime, sessionStartDate);

        sessionStored = true;
        sessionSent = false;

        delay(3000);

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
            makeLabel(nobt_box, "Connect BT to", 50, 75, LV_COLOR_BLACK, FONT_MEDIUM);
            makeLabel(nobt_box, "sync session", 58, 98, LV_COLOR_BLACK, FONT_MEDIUM);
            lv_scr_load(scr_nobt);
            lv_task_handler();
            delay(4000);
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