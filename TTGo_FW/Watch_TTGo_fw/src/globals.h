#pragma once
#include "config.h"

// Watch objects
extern TTGOClass *watch;
extern BMA *sensor;
extern PCF8563_Class *rtc;
extern TinyGPSPlus *gps;
extern BluetoothSerial SerialBT;

extern volatile uint8_t state;
extern int lastSavedIndex;

// battery variables
extern int batteryPercent;
extern bool s_isCharging;
extern unsigned long batteryTimer;

// irq variables
extern volatile bool irqBMA;
extern volatile bool irqButton;
extern volatile bool irqButtonLong;

// power variables
extern bool screenAsleep;
extern unsigned long lastActivity;

// input variables
extern float height;
extern float weight;

// distance calculation variables
extern uint32_t currentSteps;
extern uint32_t lastStep;
extern float distance_m;
extern float stride;
extern uint32_t caloriesBurned;


// session variables
extern uint32_t sessionId;
extern bool sessionStored;
extern bool sessionSent;
extern String sessionStartDate;
extern String sessionStartTime;
extern String sessionEndTime;
extern unsigned long sessionStartMs;
extern unsigned long sessionDurationMs;
extern String durationStr;


// GPS variables
struct GpsPoint {
    double lat;
    double lon;
    double alt;
};

static const int MAX_GPS_POINTS = 1000;

extern GpsPoint gpsPoints[MAX_GPS_POINTS];
extern int gpsPointCount;
extern unsigned long lastGpsSave;
extern unsigned long lastGpsFileSave;
