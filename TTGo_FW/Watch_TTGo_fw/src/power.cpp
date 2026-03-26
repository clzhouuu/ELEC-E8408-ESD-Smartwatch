#include "config.h"
#include "power.h"
#include "globals.h"

// battery
#include "math.h"

#define BATTERY_DEADBAND_THRESHOLD 20.0 // Threshold for battery percentage deadband in mV
float last_battery_percentage = -1.0; // Initialize to an invalid percentage to ensure the first reading is processed

// battery
void readBattery() {
    s_isCharging = watch->power->isVBUSPlug() || watch->power->isChargeing();
    float raw_percentage = 0.0; // buffer for percentage calculation
    float voltage_mV = watch->power->getBattVoltage();
    
    if (voltage_mV >= 4200.0) {
        raw_percentage = 100;
    } else if (voltage_mV <= 3300.0) {
        raw_percentage = 0;
    } else if (voltage_mV > 3800.0) { //4200-3800 mV section 1 100% to 40%:
    raw_percentage = 40.0 + ((voltage_mV - 3800.0) * 60.0 / 400.0);
    } else { //3800-3300 mV section 2 40% to 0%:
        raw_percentage = ((voltage_mV - 3300.0) * 40.0 / 500.0);
    }

    // Deadband filter to prevent fluctuations
    if (last_battery_percentage < 0.0) {
        // Init (bypass deadband)
        last_battery_percentage = raw_percentage; 
    } else {
        // Calculate the fluctuation
        float delta = fabs(raw_percentage - last_battery_percentage);
        
        // Update based on deadband threshold
        if (delta >= BATTERY_DEADBAND_THRESHOLD) {
            last_battery_percentage = raw_percentage;
        }
    }

    batteryPercent = constrain((int)last_battery_percentage, 0, 100);
}

// screen sleep
void screenSleep() {
    if (screenAsleep) return;
    screenAsleep = true;
    watch->closeBL();
}

// screen wake
void screenWake() {
    if (!screenAsleep) return;
    screenAsleep = false;
    watch->openBL();
    lastActivity = millis();
}

// shutdown
void shutDown() {
    Serial.println("Long press: shutting down");
    watch->closeBL();
    watch->displaySleep();
    watch->power->setPowerOutPut(AXP202_LDO2, false);
    watch->power->setPowerOutPut(AXP202_LDO3, false);
    watch->power->setPowerOutPut(AXP202_DCDC2, false);
    watch->power->setPowerOutPut(AXP202_EXTEN, false);
    watch->power->setPowerOutPut(AXP202_DCDC3, false);
    esp_deep_sleep_start();
}


void initHikeWatch() {
    if (!LITTLEFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
        Serial.println("LITTLEFS Mount Failed"); return;
    }

    Acfg cfg;
    cfg.odr = BMA4_OUTPUT_DATA_RATE_50HZ;
    cfg.range = BMA4_ACCEL_RANGE_4G;
    cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;
    cfg.perf_mode = BMA4_CONTINUOUS_MODE;
    sensor->accelConfig(cfg);
    sensor->enableAccel();
    sensor->enableFeature(BMA423_STEP_CNTR, true);
    sensor->resetStepCounter();
    sensor->enableStepCountInterrupt();

    pinMode(BMA423_INT1, INPUT);
    attachInterrupt(BMA423_INT1, [] { 
        irqBMA = true; 
    }, RISING);

    pinMode(AXP202_INT, INPUT_PULLUP);
    attachInterrupt(AXP202_INT, [] { 
        irqButton = true;
    }, FALLING);

    watch->power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ, true);
    watch->power->enableIRQ(AXP202_PEK_LONGPRESS_IRQ, true);
    watch->power->clearIRQ();

    return;
}