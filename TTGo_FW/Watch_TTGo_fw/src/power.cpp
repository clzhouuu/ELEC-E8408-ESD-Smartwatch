#include "config.h"
#include "power.h"
#include "globals.h"

// battery
void readBattery() {
    s_isCharging = watch->power->isVBUSPlug() || watch->power->isChargeing();

    if (s_isCharging) {
        batteryPercent = watch->power->getBattPercentage();
    } else {
        batteryPercent = constrain((int)((watch->power->getBattVoltage() / 1000.0f - 3.3f) / 0.9f * 100), 0, 100);
    }
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