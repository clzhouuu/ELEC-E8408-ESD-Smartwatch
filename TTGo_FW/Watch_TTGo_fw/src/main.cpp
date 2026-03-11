#include "config.h"
using fs::File;

// Check if Bluetooth configs are enabled
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// Bluetooth Serial object
BluetoothSerial SerialBT;

// Watch objects
TTGOClass *watch;
TFT_eSPI *tft;
BMA *sensor;
PCF8563_Class *rtc;
TinyGPSPlus *gps;          // GPS added

// Global variables for batteryState
int batteryPercent = 0;
unsigned long batteryTimer = 0;

uint32_t sessionId = 30;
unsigned long last = 0;
unsigned long updateTimeout = 0;

volatile uint8_t state;
volatile bool irqBMA = false;
volatile bool irqButton = false;

// input variables
float height = 1.65;
float weight = 50.0f;

// distance calculation variables
uint32_t steps = 0;
uint32_t lastStep = 0;
uint32_t currentSteps = 0;
float distance_m = 0.0f;
const float stride = 0.43 * height;

// time variables
String sessionStartDate = "";
String sessionStartTime = "";
String sessionEndTime = "";
unsigned long sessionStartMs = 0;
unsigned long sessionDurationMs = 0;

// calorie estimation variables
uint32_t MET = 6;
uint32_t caloriesBurned = 0;

bool sessionStored = false;
bool sessionSent = false;

// GPS variables
struct GpsPoint {
    double lat;
    double lon;
    unsigned long ms;
};

const int MAX_GPS_POINTS = 500;
GpsPoint gpsPoints[MAX_GPS_POINTS];
int gpsPointCount = 0;
unsigned long lastGpsSave = 0;      // throttles point capture (every 3s)
unsigned long lastGpsFileSave = 0;  // throttles file write (every 15s)

void initHikeWatch()
{
    // LittleFS
    if(!LITTLEFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
        Serial.println("LITTLEFS Mount Failed");
        return;
    }
    
    // Stepcounter
    // Configure IMU
    Acfg cfg;
    cfg.odr = BMA4_OUTPUT_DATA_RATE_100HZ;
    cfg.range = BMA4_ACCEL_RANGE_2G;
    cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;
    cfg.perf_mode = BMA4_CONTINUOUS_MODE;

    sensor->accelConfig(cfg);
    sensor->enableAccel(); 

    // Enable BMA423 step count feature
    sensor->enableFeature(BMA423_STEP_CNTR, true);

    // Reset steps
    sensor->resetStepCounter();

    // Turn on step interrupt
    sensor->enableStepCountInterrupt();

    // BMA interrupt
    pinMode(BMA423_INT1, INPUT);
    attachInterrupt(BMA423_INT1, [] { 
        irqBMA = true; 
    }, RISING);

    // Side button
    pinMode(AXP202_INT, INPUT_PULLUP);
    attachInterrupt(AXP202_INT, [] {
        irqButton = true;
    }, FALLING);

    //!Clear IRQ unprocessed first
    watch->power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ, true);
    watch->power->clearIRQ();

    return;
}

void sendDataBT(fs::FS &fs, const char * path)
{
    /* Sends data via SerialBT */
    fs::File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }
    Serial.println("- read from file:");
    while(file.available()){
        SerialBT.write(file.read());
    }
    file.close();
}

void sendSessionBT()
{
    // Read session and send it via SerialBT
    watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
    watch->tft->drawString("Sending session", 20, 80);
    watch->tft->drawString("to Hub", 80, 110);

    // Sending session id
    sendDataBT(LITTLEFS, "/id.txt");
    SerialBT.write(';');
    // Sending steps
    sendDataBT(LITTLEFS, "/steps.txt");
    SerialBT.write(';');
    // Sending distance
    sendDataBT(LITTLEFS, "/distance.txt");
    SerialBT.write(';');
    // Send connection termination char
    SerialBT.write('\n');
}

void saveIdToFile(uint16_t id)
{
    char buffer[10];
    itoa(id, buffer, 10);
    writeFile(LITTLEFS, "/id.txt", buffer);
}

void saveStepsToFile(uint32_t step_count)
{
    char buffer[10];
    itoa(step_count, buffer, 10);
    writeFile(LITTLEFS, "/steps.txt", buffer);
}

void saveDistanceToFile(float distance)
{
    char buffer[16];
    dtostrf(distance, 6, 3, buffer);
    writeFile(LITTLEFS, "/distance.txt", buffer);
}

void deleteSession()
{
    deleteFile(LITTLEFS, "/id.txt");
    deleteFile(LITTLEFS, "/distance.txt");
    deleteFile(LITTLEFS, "/steps.txt");
    deleteFile(LITTLEFS, "/coord.txt");
}

// GPS - log a point every 3 seconds if valid fix
void logGps() {
    watch->gpsHandler();

    if (!gps) return;
    if (!gps->location.isValid() || !gps->location.isUpdated()) return;
    if (millis() - lastGpsSave < 3000) return;

    lastGpsSave = millis();

    if (gpsPointCount < MAX_GPS_POINTS) {
        gpsPoints[gpsPointCount++] = {
            gps->location.lat(),
            gps->location.lng(),
            millis() - sessionStartMs
        };
    }
}

// GPS - write all captured points to file
void saveGpsPointsToFile() {
    File file = LITTLEFS.open("/coord.txt", FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open /coord.txt");
        return;
    }
    for (int i = 0; i < gpsPointCount; i++) {
        file.print(gpsPoints[i].lat, 6);
        file.print(",");
        file.print(gpsPoints[i].lon, 6);
        file.print(",");
        file.println(gpsPoints[i].ms);
    }
    file.close();
}

void setup()
{
    Serial.begin(115200);
    watch = TTGOClass::getWatch();
    watch->begin();
    watch->openBL();

    //Receive objects for easy writing
    tft = watch->tft;
    sensor = watch->bma;
    rtc = watch->rtc;

    rtc->check();

    // GPS
    watch->trunOnGPS();
    watch->gps_begin();
    gps = watch->gps;

    initHikeWatch();

    state = 3;

    SerialBT.begin("Hiking Watch");
    Serial.println("Bluetooth started"); //show indication
}

void drawTime(){
    static unsigned long lastUpdate = 0;

    if (millis() - lastUpdate > 1000) {
        lastUpdate = millis();
        watch->tft->fillRect(170, 5, 70, 20, TFT_BLACK); 
        watch->tft->drawString(rtc->formatDateTime(PCF_TIMEFORMAT_HM), 170, 5);
    }
}

void loop()
{
    drawTime();
    
    switch (state)
    {
    case 1:
    {
        drawTime();

        /* Initial stage */
        //Basic interface
        watch->tft->fillScreen(TFT_BLACK);
        watch->tft->setTextFont(4);
        watch->tft->setTextColor(TFT_WHITE, TFT_BLACK);
        watch->tft->drawString("Hiking Watch",  45, 25, 4);
        watch->tft->drawString("Press button", 50, 80);
        watch->tft->drawString("to start session", 40, 110);

        // variable initiation
        bool exitSync = false;

        // Bluetooth discovery
        while (1)
        {
            /* Bluetooth sync */
            if (SerialBT.available())
            {
                char incomingChar = SerialBT.read();
                if (incomingChar == 'c' and sessionStored and not sessionSent)
                {
                    sendSessionBT();
                    sessionSent = true;
                }
                if (sessionSent && sessionStored) {
                    // Update timeout before blocking while
                    updateTimeout = 0;
                    last = millis();
                    while(1)
                    {
                        updateTimeout = millis();
                        if (SerialBT.available())
                            incomingChar = SerialBT.read();
                        if (incomingChar == 'r')
                        {
                            Serial.println("Got an R");
                            // Delete session
                            deleteSession();
                            sessionStored = false;
                            sessionSent = false;
                            incomingChar = 'q';
                            exitSync = true;
                            break;
                        }
                        else if ((millis() - updateTimeout > 2000))
                        {
                            Serial.println("Waiting for timeout to expire");
                            updateTimeout = millis();
                            sessionSent = false;
                            exitSync = true;
                            break;
                        }
                    }
                }
            }
            if (exitSync)
            {
                delay(1000);
                watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
                watch->tft->drawString("Hiking Watch",  45, 25, 4);
                watch->tft->drawString("Press button", 50, 80);
                watch->tft->drawString("to start session", 40, 110);
                exitSync = false;
            }

            /*      IRQ     */
            if (irqButton) {
                irqButton = false;
                watch->power->readIRQ();
                if (state == 1)
                {
                    state = 2;
                }
                watch->power->clearIRQ();
            }
            if (state == 2) {
                if (sessionStored)
                {
                    watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
                    watch->tft->drawString("Overwriting",  55, 100, 4);
                    watch->tft->drawString("session", 70, 130);
                    delay(1000);
                }
                break;
            }
        }
        break;
    }

    case 2:
    {
        /* Hiking session initialisation */
        drawTime();

        // Reset GPS state for new session
        deleteFile(LITTLEFS, "/coord.txt");
        gpsPointCount   = 0;
        lastGpsSave     = 0;
        lastGpsFileSave = 0;

        state = 3;
        break;
    }

    case 3:
    {
        /* Hiking session ongoing */
        drawTime();

        sessionStartDate = String(rtc->formatDateTime(PCF_TIMEFORMAT_DD_MM_YYYY));
        sessionStartTime = String(rtc->formatDateTime(PCF_TIMEFORMAT_HMS));
        sessionStartMs = millis();

        watch->tft->fillScreen(TFT_BLACK);
        watch->tft->setTextColor(TFT_WHITE, TFT_BLACK);
        watch->tft->setTextFont(2); 

        watch->tft->drawString("Starting hike", 55, 100, 2);
        delay(2000);
        watch->tft->fillScreen(TFT_BLACK);

        // Initial display
        watch->tft->drawString("Steps:",    20, 50, 2);
        watch->tft->drawString("Dist:",     20, 80, 2);
        watch->tft->drawString("Calories:", 20, 110, 2);
        watch->tft->drawString("Duration:", 20, 140, 2);
        watch->tft->drawString("Battery:",  20, 170, 2);

        //reset step-counter
        sensor->resetStepCounter();
        lastStep = 0;
        distance_m = 0.0f;
        currentSteps = 0;
        last = millis();
        updateTimeout = 0;

        watch->tft->drawString("0",         130, 50, 2);
        watch->tft->drawString("0.00 km",   130, 80, 2);
        watch->tft->drawString("0 kcal",    130, 110, 2);
        watch->tft->drawString("00:00:00",  130, 140, 2);
        watch->tft->drawString("0 %",       130, 170, 2);

        // loop
        while (state == 3) {

            // time display
            drawTime();

            // GPS - log point and periodically flush to file
            logGps();
            if (millis() - lastGpsFileSave > 15000) {
                lastGpsFileSave = millis();
                saveGpsPointsToFile();
            }
        
            static unsigned long lastDurationUpdate = 0;
            if (millis() - lastDurationUpdate > 1000) {
                lastDurationUpdate = millis();
                sessionDurationMs = millis() - sessionStartMs;

                unsigned long totalSeconds = sessionDurationMs / 1000;
                unsigned long seconds = totalSeconds % 60;
                unsigned long minutes = (totalSeconds % 3600) / 60;
                unsigned long hours = totalSeconds / 3600;

                watch->tft->fillRect(130, 140, 90, 16, TFT_BLACK);
                watch->tft->setCursor(130, 140);

                if (hours < 10) watch->tft->print("0");
                watch->tft->print(hours);
                watch->tft->print(":");
                if (minutes < 10) watch->tft->print("0");
                watch->tft->print(minutes);
                watch->tft->print(":");
                if (seconds < 10) watch->tft->print("0");
                watch->tft->print(seconds);
            }

            if (irqBMA) {
                irqBMA = false;
                sensor->readInterrupt();
                if (sensor->isStepCounter()) {
                    // steps
                    currentSteps = sensor->getCounter(); 
                    uint32_t delta = currentSteps - lastStep;
                    lastStep = currentSteps;

                    // distance
                    distance_m += delta * stride; 
                    
                    // calories
                    caloriesBurned = (MET * weight * sessionDurationMs) / 3600000;

                    watch->tft->fillRect(130, 50, 80, 16, TFT_BLACK);
                    watch->tft->drawString(String(currentSteps), 130, 50, 2);

                    watch->tft->fillRect(130, 80, 90, 16, TFT_BLACK);
                    watch->tft->drawString(String(distance_m / 1000.0f, 2) + " km", 130, 80, 2);

                    watch->tft->fillRect(130, 110, 90, 16, TFT_BLACK);
                    watch->tft->drawString(String(caloriesBurned) + " kcal", 130, 110, 2);
                }
            }

            if (millis() - batteryTimer > 5000) {
                batteryTimer = millis();

                if (watch->power->isChargeing()) {
                    batteryPercent = watch->power->getBattPercentage();
                } else {
                    float v = watch->power->getBattVoltage() / 1000.0f;
                    batteryPercent = constrain((int)((v - 3.3f) / (4.2f - 3.3f) * 100), 0, 100);
                }

                watch->tft->fillRect(130, 170, 80, 16, TFT_BLACK);
                watch->tft->drawString(String(batteryPercent) + "%", 130, 170);

                if (batteryPercent < 20) {
                    watch->tft->setTextColor(TFT_RED, TFT_BLACK);
                    watch->tft->drawString("LOW BATTERY!", 40, 190);
                    watch->tft->setTextColor(TFT_WHITE, TFT_BLACK);
                } else {
                    watch->tft->fillRect(40, 190, 140, 16, TFT_BLACK);
                }
            }
        

            // --- Button interrupt to end hike ---
            if (irqButton)
            {
                sessionEndTime = String(rtc->formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S));
                sessionDurationMs = millis() - sessionStartMs;
                irqButton = false;
                state = 4;
            }
        }
        break;      
    }

    case 4:
    {
        // Save hiking session data
        saveGpsPointsToFile();      // flush remaining GPS points
        saveIdToFile(sessionId);
        saveStepsToFile(currentSteps);
        saveDistanceToFile(distance_m / 1000.0f);

        sessionStored = true;
        sessionSent = false;
        
        state = 1;  
        break;
    }

    default:
        // Restart watch
        ESP.restart();
        break;
    }
}