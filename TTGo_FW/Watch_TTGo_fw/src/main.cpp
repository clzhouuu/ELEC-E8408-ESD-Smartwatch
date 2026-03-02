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

uint32_t sessionId = 30;

// Global variables for stepcount
uint32_t currentSteps = 0;
bool hikeActive = false;

// Global variables for batteryState
int batteryPercent = 0;
unsigned long batteryTimer = 0;


unsigned long last = 0;
unsigned long updateTimeout = 0;


volatile uint8_t state;
volatile bool irqBMA = false;
volatile bool irqButton = false;

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
    // Enable BMA423 step count feature (the hardware embedded in the Watch)
    sensor->begin();
    sensor->enableFeature(BMA423_STEP_CNTR, true);
    // Reset steps
    sensor->resetStepCounter();
    // Turn on step interrupt
    
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
    
    // case 1: // idle state in FSM diagram
    // {
    //     /* Initial stage */
    //     //Basic interface
    //     watch->tft->fillScreen(TFT_BLACK);
    //     watch->tft->setTextFont(4);
    //     watch->tft->setTextColor(TFT_WHITE, TFT_BLACK);
    //     watch->tft->drawString("Hiking Watch",  45, 25, 4);
    //     watch->tft->drawString("Press button", 50, 80);
    //     watch->tft->drawString("to start session", 40, 110);

    //     bool exitSync = false;

    //     //Bluetooth discovery
    //     while (1)
    //     {
    //         /* Bluetooth sync */
    //         if (SerialBT.available())
    //         {
    //             char incomingChar = SerialBT.read();
    //             if (incomingChar == 'c' and sessionStored and not sessionSent)
    //             {
    //                 sendSessionBT();
    //                 sessionSent = true;
    //             }

    //             if (sessionSent && sessionStored) {
    //                 // Update timeout before blocking while
    //                 updateTimeout = 0;
    //                 last = millis();
    //                 while(1)
    //                 {
    //                     updateTimeout = millis();

    //                     if (SerialBT.available())
    //                         incomingChar = SerialBT.read();
    //                     if (incomingChar == 'r')
    //                     {
    //                         Serial.println("Got an R");
    //                         // Delete session
    //                         deleteSession();
    //                         sessionStored = false;
    //                         sessionSent = false;
    //                         incomingChar = 'q';
    //                         exitSync = true;
    //                         break;
    //                     }
    //                     else if ((millis() - updateTimeout > 2000))
    //                     {
    //                         Serial.println("Waiting for timeout to expire");
    //                         updateTimeout = millis();
    //                         sessionSent = false;
    //                         exitSync = true;
    //                         break;
    //                     }
    //                 }
    //             }
    //         }
    //         if (exitSync)
    //         {
    //             delay(1000);
    //             watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
    //             watch->tft->drawString("Hiking Watch",  45, 25, 4);
    //             watch->tft->drawString("Press button", 50, 80);
    //             watch->tft->drawString("to start session", 40, 110);
    //             exitSync = false;
    //         }

    //         /*      IRQ     */
    //         if (irqButton) {
    //             irqButton = false;
    //             watch->power->readIRQ();
    //             if (state == 1)
    //             {
    //                 state = 2;
    //             }
    //             watch->power->clearIRQ();
    //         }
    //         if (state == 2) {
    //             if (sessionStored)
    //             {
    //                 watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
    //                 watch->tft->drawString("Overwriting",  55, 100, 4);
    //                 watch->tft->drawString("session", 70, 130);
    //                 delay(1000);
    //             }
    //             break;
    //         }
    //     }
    //     break;
    // }
    
    case 1:
    {
        /* Hiking session initalisation */
        
        //reset stepcounter at the start of hike
        sensor->resetStepCounter();
        hikeActive = true;
        state = 3;

        break;
    }
    case 3:
    {
        /* Hiking session ongoing */

        if (!hikeActive) break;

        // OG code
        /*
        watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
        watch->tft->drawString("Starting hike", 45, 100);
        delay(1000);
        watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);

        watch->tft->setCursor(45, 70);
        watch->tft->print("Steps: 0");

        watch->tft->setCursor(45, 100);
        watch->tft->print("Dist: 0 km");

        last = millis();
        updateTimeout = 0;
        */

        
        // New code - Update display every 1 second
        if (millis() - last > 1000) {
            last = millis();
            
            currentSteps = sensor->getCounter(); // read step count

            // Update battery % every 5 seconds
            if (millis() - batteryTimer > 5000);{
                batteryTimer = millis();
                batteryPercent = watch->power->getBattPercentage();
                
                //If low battery
                if (batteryPercent < 20) {
                    watch->tft->setCursor(40, 200);
                    watch->tft->setTextColor(TFT_RED, TFT_BLACK);
                    watch->tft->printf("LOW BATTERY!");
                    watch->tft->setTextColor(TFT_WHITE, TFT_BLACK);
                }
            }
            // Clear screen area
            watch->tft->fillScreen(TFT_BLACK);
            
            // Display step count
            watch->tft->setCursor(40, 80);
            watch->tft->setTextSize(2);
            watch->tft->print("Steps:");
            watch->tft->setCursor(40, 110);
            watch->tft->setTextSize(3);
            watch->tft->printf("Steps: %d", currentSteps);

            // Display Battery %
            watch->tft->setCursor(40, 160);
            watch->tft->setTextSize(2);
            watch->tft->printf("Battery: %d", batteryPercent, "%");
     

        }
        break;
    }
    case 4:
    {
        //Save hiking session data
        delay(1000);
        state = 1;  
        
        uint32_t finalSteps = sensor->getCounter();
        saveStepsToFile(finalSteps);

        break;
    }
    default:
        // Restart watch
        ESP.restart();
        break;
    }
}