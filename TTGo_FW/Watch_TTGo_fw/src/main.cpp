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
float height = 0.00165;
float weight = 0.0f;

// distance calculation variables
uint32_t steps = 0;
uint32_t lastStep = 0;
float distance_m = 0.0f;
const float stride = 0.43 * height;

bool sessionStored = false;
bool sessionSent = false;

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

    // BMA interupt
    pinMode(BMA423_INT1, INPUT);
    attachInterrupt(BMA423_INT1, [] { 
        irqBMA = true; 
    }, RISING);


    // GPS
    // Configure IMU

    // Enable BMA423 step count feature

    // Reset steps

    // Turn on step interrupt

    // BMA interupt



    // Pop-up messages
    // Tumbling

    // hi sophia

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
    char buffer[10];
    itoa(distance, buffer, 10);
    writeFile(LITTLEFS, "/distance.txt", buffer);
}

void deleteSession()
{
    deleteFile(LITTLEFS, "/id.txt");
    deleteFile(LITTLEFS, "/distance.txt");
    deleteFile(LITTLEFS, "/steps.txt");
    deleteFile(LITTLEFS, "/coord.txt");
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
    
    initHikeWatch();

    state = 1;

    SerialBT.begin("Hiking Watch");
}

void loop()
{
    switch (state)
    {
    case 1:
    {
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

        //Bluetooth discovery
        
        // while (1)
        // {
        //     /* Bluetooth sync */
        //     if (SerialBT.available())
        //     {
        //         char incomingChar = SerialBT.read();
        //         if (incomingChar == 'c' and sessionStored and not sessionSent)
        //         {
        //             sendSessionBT();
        //             sessionSent = true;
        //         }

        //         if (sessionSent && sessionStored) {
        //             // Update timeout before blocking while
        //             updateTimeout = 0;
        //             last = millis();
        //             while(1)
        //             {
        //                 updateTimeout = millis();

        //                 if (SerialBT.available())
        //                     incomingChar = SerialBT.read();
        //                 if (incomingChar == 'r')
        //                 {
        //                     Serial.println("Got an R");
        //                     // Delete session
        //                     deleteSession();
        //                     sessionStored = false;
        //                     sessionSent = false;
        //                     incomingChar = 'q';
        //                     exitSync = true;
        //                     break;
        //                 }
        //                 else if ((millis() - updateTimeout > 2000))
        //                 {
        //                     Serial.println("Waiting for timeout to expire");
        //                     updateTimeout = millis();
        //                     sessionSent = false;
        //                     exitSync = true;
        //                     break;
        //                 }
        //             }
        //         }
        //     }
        //     if (exitSync)
        //     {
        //         delay(1000);
        //         watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
        //         watch->tft->drawString("Hiking Watch",  45, 25, 4);
        //         watch->tft->drawString("Press button", 50, 80);
        //         watch->tft->drawString("to start session", 40, 110);
        //         exitSync = false;
        //     }

        //     /*      IRQ     */
        //     if (irqButton) {
        //         irqButton = false;
        //         watch->power->readIRQ();
        //         if (state == 1)
        //         {
        //             state = 2;
        //         }
        //         watch->power->clearIRQ();
        //     }
        //     if (state == 2) {
        //         if (sessionStored)
        //         {
        //             watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
        //             watch->tft->drawString("Overwriting",  55, 100, 4);
        //             watch->tft->drawString("session", 70, 130);
        //             delay(1000);
        //         }
        //         break;
        //     }
        // }
        // break;
        
    }
    case 2:
    {
        /* Hiking session initalisation */
        
        state = 3;
        break;
    }
    case 3:
    {
        /* Hiking session ongoing */

        watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
        watch->tft->drawString("Starting hike", 45, 100);
        delay(2000);
        watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);

        watch->tft->setCursor(40, 70);
        watch->tft->print("Steps: ");

        watch->tft->setCursor(40, 100);
        watch->tft->print("Dist: ");

        watch->tft->setCursor(40, 130);
        watch->tft->print("Battery: ");

        last = millis();
        updateTimeout = 0;

        //reset step-counter
        sensor->resetStepCounter();
        lastStep = 0;
        distance_m  = 0.0f;

        uint32_t currentSteps = 0;

        // loop
        while (state == 3){
            if (irqBMA){
                irqBMA = false;

                sensor->readInterrupt();
                if (sensor->isStepCounter()){
                    currentSteps = sensor->getCounter();

                    uint32_t delta = currentSteps - lastStep;
                    lastStep = currentSteps;

                    distance_m += delta * stride;  

                    watch->tft->fillRect(120, 70, 100, 20, TFT_BLACK);
                    watch->tft->setCursor(130, 70);
                    watch->tft->print(currentSteps);

                    watch->tft->fillRect(120, 100, 100, 20, TFT_BLACK);
                    watch->tft->setCursor(130, 100);
                    watch->tft->print(distance_m / 1000.0f, 2);
                    watch->tft->print(" km");
                }

                if (millis() - batteryTimer > 5000){
                    batteryTimer = millis();
                    batteryPercent = watch->power->getBattPercentage();

                    // Display Battery %
                    watch->tft->fillRect(120, 130, 100, 20, TFT_BLACK);
                    watch->tft->setCursor(130, 130);
                    watch->tft->printf("%d%%", batteryPercent);
                
                    //If low battery
                    if (batteryPercent < 20) {
                        watch->tft->setCursor(40, 200);
                        watch->tft->setTextColor(TFT_RED, TFT_BLACK);
                        watch->tft->printf("LOW BATTERY!");
                        watch->tft->setTextColor(TFT_WHITE, TFT_BLACK);
                    }
                
                }

            }

            // --- Button interrupt to end hike ---
            if (irqButton)
            {
                irqButton = false;
                state = 4;
            }
        }
        break;      

    }
    case 4:
    {
        //Save hiking session data
        delay(1000);
        state = 1;  
        break;
    }
    default:
        // Restart watch
        ESP.restart();
        break;
    }
}